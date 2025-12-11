#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>

#define FLAG_IMPLEMENTATION
#include "flag.h"
#include "mongoose/mongoose.h"

#include "splashes.h"

#define NOB_IMPLEMENTATION
#include "nob.h"
#undef NOB_IMPLEMENTATION

#define ENUMS_IMPLEMENTATION
#include "enums.h"
#undef ENUMS_IMPLEMENTATION

#define BINARY_RW_IMPLEMENTATION
#include "binary_rw.h"
#undef BINARY_RW_IMPLEMENTATION

// --- UTILS ---

void RandomBytes(void *buf, size_t len) {
	int fd = open("/dev/urandom", O_RDONLY);
	assert(fd >= 0);
	size_t n = read(fd, buf, len);
	assert(n == len);
	close(fd);
}

// --- GLOBALS ---

size_t active_conns = 0;

// --- SSR ---

typedef struct SSRData {
	struct mg_http_message* hm;
	char lang_is_ru;
	size_t active_conns;
} SSRData;

#define SSR_OUT(CSTR_) nob_sb_appendf(ssr_sb, "%s", CSTR_);
#define SSR_PRINTF(...) nob_sb_appendf(ssr_sb, __VA_ARGS__);

#define SSR_PRINT_LINK(URL_) \
	do { \
		SSR_PRINTF("%s", URL_); \
		SSR_PRINTF("%s", ssr_data.hm->query.len > 0 ? "?" : ""); \
		SSR_PRINTF("%.*s", ssr_data.hm->query.len, ssr_data.hm->query.buf); \
	} while (0);
#define SSR_RU_PRINTF(...) \
	if (ssr_data.lang_is_ru) { SSR_PRINTF(__VA_ARGS__); }
#define SSR_EN_PRINTF(...) \
	if (!ssr_data.lang_is_ru) { SSR_PRINTF(__VA_ARGS__); }
#define SSR_DEREF_DATA(d) \
	struct ssr_data ssr_data = (struct ssr_data*)ssr_user ? *((struct ssr_data*)ssr_user) : (struct ssr_data){0};

#include "ssr_generated/ssr_template_default_before.h"
#include "ssr_generated/ssr_template_default_after.h"
#include "ssr_generated/ssr_root.h"
#include "ssr_generated/ssr_about.h"
#include "ssr_generated/ssr_page404.h"
#include "ssr_generated/ssr_proj_mna.h"
#include "ssr_generated/ssr_proj_askme.h"

// --- APP ---

struct a_config {
	int port;
	char* web_dir;
} aconf;

void a_parse_flags(int argc, char** argv) {
	mg_log_set(MG_LL_NONE);

	bool* f_help = flag_bool("help", 0, "help");
	uint64_t* f_ll = flag_uint64("log-level", 0, "none, error, info, debug, verbose (0, 1, 2, 3, 4)");
	uint64_t* f_port = flag_uint64("port", 6969, "port for the server");
	char** f_web_dir = flag_str("webdir", "./web", "directory for the server");

	if (!flag_parse(argc, argv)) {
    flag_print_options(stdout);
		flag_print_error(stderr);
		exit(1);
	}

	if (*f_help) {
    flag_print_options(stdout);
		exit(0);
	}

	aconf.web_dir = *f_web_dir;
	aconf.port = *f_port;
	mg_log_set(*f_ll);
}

// --- EVENTS ---

// -- Visitors --

#define VISITOR_PENDING_SECONDS 5
#define VISITOR_ACTIVE_SECONDS 360

typedef struct Visitor {
	uint8_t cookie_id[16];
	time_t time_created;
	time_t time_last_active;
	unsigned is_pending : 1;
} Visitor;

typedef struct {
	size_t count;
	size_t capacity;
	Visitor* items;
} VisitorArray;

VisitorArray visitors;

Visitor* VisitorsAddPending(VisitorArray* visitors) {
	Visitor visitor = {0};
	visitor.is_pending = 1;
	visitor.time_created = time(NULL);
	visitor.time_last_active = time(NULL);
	RandomBytes(visitor.cookie_id, 16);
	uint8_t* b = visitor.cookie_id;
	MG_DEBUG(("id=%02x%02x*****%02x", b[0], b[1], b[15]));
	nob_da_append(visitors, visitor);
	return visitors->items + visitors->count - 1;
}

Visitor* VisitorsGetByCookieID(VisitorArray* visitors, void* cookie_id) {
	nob_da_foreach(Visitor, visitor, visitors) {
		if (memcmp(visitor->cookie_id, cookie_id, 16) == 0)
			return visitor;
	}
	return NULL;
}

bool VisitorIsActive(Visitor* visitor);
inline bool VisitorIsActive(Visitor* visitor) {
	if (time(NULL) > visitor->time_last_active+VISITOR_ACTIVE_SECONDS) {
		return false;
	}
	return true;
}

void VisitorsManageUnactive(VisitorArray* visitors) {
	size_t i = 0;
	active_conns = 0;
	nob_da_foreach(Visitor, visitor, visitors) {
		if (visitor->is_pending) {
			if (time(NULL) > visitor->time_created+VISITOR_PENDING_SECONDS) {
				nob_da_remove_unordered(visitors, i);
				MG_INFO(("expired.\n"));
				return;
			}
		}
		if (!visitor->is_pending && VisitorIsActive(visitor))
			active_conns++;
		i++;
	}
}

// -- HTTP --

Visitor* HTTPProcessVisitor(struct mg_http_message* hm) {
	struct mg_str* header_cookie = mg_http_get_header(hm, "Cookie");
	if (header_cookie) {
		// Process cookie
		struct mg_str var_id = mg_http_get_header_var(*header_cookie, mg_str("id"));
		if (var_id.len == 0 || var_id.len != 32) {
			MG_DEBUG(("cookie id length %d is invalid", var_id.len));
			return NULL;
		}
		uint8_t cookie_id[16];
		for (size_t i = 0; i < 16; i++) {
			int flag = sscanf(var_id.buf+i*2, "%2hhx", &cookie_id[i]);
			if (flag != 1) {
				MG_DEBUG(("parsing error."));
				return NULL;
			}
		}
		uint8_t* b = cookie_id;
		MG_DEBUG(("id=%02x%02x*****%02x", b[0], b[1], b[15]));
		// Check if exists
		Visitor* visitor = VisitorsGetByCookieID(&visitors, cookie_id);
		if (visitor == NULL) {
			MG_DEBUG(("visitor not found."));
			return NULL;
		}
		MG_DEBUG(("visitor found."));
		if (visitor->is_pending) {
			MG_DEBUG(("visitor connection upgrade."));
			visitor->is_pending = 0;
		}
		visitor->time_last_active = time(NULL);
		return visitor;
	}
	return NULL;
}

typedef void (*SSRFuncPtr)(Nob_String_Builder*, SSRData);

char URICompare(struct mg_str uri, struct mg_str exp) {
	if (uri.len < exp.len) return 0;
	if (strncmp(uri.buf, exp.buf, exp.len) != 0) return 0;
	for (size_t i = exp.len; i < uri.len; i++) { // Trailing '/'
		if (uri.buf[i] != '/') return 0;
	}
	return 1;
}

#define SSR_MATCH(URI_, FUNC_) \
	if (URICompare(hm->uri, mg_str(URI_))) return FUNC_;

SSRFuncPtr HTTPServePage(struct mg_connection* c, struct mg_http_message* hm) {
	SSR_MATCH("/", ssr_root);
	SSR_MATCH("/home", ssr_root);
	SSR_MATCH("/about", ssr_about);
	SSR_MATCH("/mna", ssr_proj_mna);
	SSR_MATCH("/askme", ssr_proj_askme);
	for (size_t i = 0; i < hm->uri.len; i++)
		if (hm->uri.buf[i] == '.') return NULL;
	return ssr_page404;
}

char HTTPIsLangRu(struct mg_http_message* hm) {
	struct mg_str var_lang = mg_http_var(hm->query, mg_str("lang"));
	if (var_lang.len >= 2)
		return strncmp("ru", var_lang.buf, 2) == 0;
	struct mg_str* header_lang = mg_http_get_header(hm, "Accept-Language");
	if (header_lang && header_lang->len >= 2)
		return strncmp("ru", header_lang->buf, 2) == 0;
	return 0;
}

Visitor* HTTPAddPendingVisitor(Nob_String_Builder* resp_headers) {
	Visitor* visitor = VisitorsAddPending(&visitors);
	if (visitor == NULL) return NULL;
	nob_sb_appendf(resp_headers, "Set-Cookie: id=");
	for (size_t i = 0; i < 16; i++)
		nob_sb_appendf(resp_headers, "%02x", visitor->cookie_id[i]);
	nob_sb_appendf(resp_headers, "\n");
	return visitor;
}

void HandleHTTPMessage(struct mg_connection* c, void* ev_data) {

	struct mg_http_message* hm = (struct mg_http_message*)ev_data;

	if (mg_strcmp(hm->uri, mg_str("/ws")) == 0) {
		mg_ws_upgrade(c, hm, NULL);
		return;
	}

	Nob_String_Builder sb = {0};
	Nob_String_Builder resp_headers = {0};

	Visitor* visitor = HTTPProcessVisitor(hm);
	if (visitor == NULL) {
		visitor = HTTPAddPendingVisitor(&resp_headers);
		if (visitor == NULL) goto cleanup;
	}

	if (!strncmp(hm->method.buf, "GET", 3)) {

		if (mg_strcmp(hm->uri, mg_str("/enums.js")) == 0) {
			mg_http_reply(c, 200, "", enums_js.items);
			return;
		}

		SSRFuncPtr ssr_func_ptr = HTTPServePage(c, hm);
		if (!ssr_func_ptr) {
			struct mg_http_serve_opts opts = { .root_dir = aconf.web_dir };
			mg_http_serve_dir(c, hm, &opts);
			goto cleanup;
		}

		SSRData ssr_data = {0};
		ssr_data.hm = hm;
		ssr_data.lang_is_ru = HTTPIsLangRu(hm);
		ssr_data.active_conns = active_conns;

		nob_da_reserve(&sb, 8192);

		(*ssr_func_ptr)(&sb, ssr_data);

		nob_da_append(&sb, '\0');
		nob_da_append(&resp_headers, '\0');

		mg_http_reply(c, 200, resp_headers.items, sb.items);
	}
cleanup:
	nob_sb_free(resp_headers);
	nob_sb_free(sb);
}

void HandleWSMessage(struct mg_connection* c, void* ev_data) {
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	BReader br = {0};
	br.count = wm->data.len;
	br.data = wm->data.buf;
	uint16_t gcmt;
	mg_hexdump(br.data, br.count);
	if (!BReadU16(&br, &gcmt)) { return; }
	switch (gcmt) {
		case CME_ASKME_QUESTION:
			uint64_t nanos = nob_nanos_since_unspecified_epoch();
			printf("nanos=%lu\n", nanos);
			if (br.count > 256) { return; }
			nob_write_entire_file(nob_temp_sprintf("dbs/askme/%lu", nanos), br.data + br.i, br.count - br.i);
			nob_temp_reset();
			break;
	}
}

void EventHandler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_WS_MSG:
			HandleWSMessage(c, ev_data);
			break;
		case MG_EV_HTTP_MSG:
			HandleHTTPMessage(c, ev_data);
			break;
		case MG_EV_POLL:
			VisitorsManageUnactive(&visitors);
			break;
	}
}

// --- MAIN ---

char is_working = 1;

void app_terminate(int sig) { is_working = 0; }

int main(int argc, char* argv[]) {
	EnumsGenerateJS();
	nob_mkdir_if_not_exists("dbs");
	nob_mkdir_if_not_exists("dbs/askme");

	a_parse_flags(argc, argv);

	printf("log_level: %d\n", mg_log_level);
	printf("aconf.web_dir: %s\n", aconf.web_dir);
	printf("aconf.port: %d\n", aconf.port);

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	char addrstr[32];
	snprintf(addrstr, sizeof(addrstr), "http://0.0.0.0:%d", aconf.port);
	mg_http_listen(&mgr, addrstr, EventHandler, NULL);

	signal(SIGINT, app_terminate);
	signal(SIGTERM, app_terminate);

	while (is_working) {
		mg_mgr_poll(&mgr, 1000);
	}

	// Closing
	mg_mgr_free(&mgr);
	printf("Server closed.\n");

	return 0;
}
