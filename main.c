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

#include "credentials.h" /* Created by user */

#define TGBOT_IMPLEMENTATION
#include "tgbot.h"
#undef TGBOT_IMPLEMENTATION

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

#define SSR_IMPLEMENTATION
#include "ssr.h"
#undef SSR_IMPLEMENTATION

#include "ssr_generated/ssr_template_default_before.h"
#include "ssr_generated/ssr_template_default_after.h"
#include "ssr_generated/ssr_root.h"
#include "ssr_generated/ssr_about.h"
#include "ssr_generated/ssr_page404.h"
#include "ssr_generated/ssr_proj_mna.h"
#include "ssr_generated/ssr_proj_askme.h"

#define VISITORS_IMPLEMENTATION
#include "visitors.h"
#undef VISITORS_IMPLEMENTATION

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

// -- HTTP --

typedef void (*SSRFuncPtr)(Nob_String_Builder*, SSRData);

char URICompare(struct mg_str uri, struct mg_str exp) {
	if (uri.len < exp.len) return 0;
	if (strncmp(uri.buf, exp.buf, exp.len) != 0) return 0;
	for (size_t i = exp.len; i < uri.len; i++) { // Trailing '/'
		if (uri.buf[i] != '/') return 0;
	}
	return 1;
}

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

typedef struct ConnData {
	uint64_t last_msg_nanos;
} ConnData;

bool ConnCooldown(struct mg_connection* c) {
	ConnData* cd = (ConnData*)c->fn_data;
	uint64_t curr_sec = nob_nanos_since_unspecified_epoch() / NOB_NANOS_PER_SEC;
	uint64_t last_sec = cd->last_msg_nanos / NOB_NANOS_PER_SEC;
	bool result = curr_sec > last_sec + 10.0;
	if (result) { cd->last_msg_nanos = nob_nanos_since_unspecified_epoch(); }
	return result;
}

// TODO: use JSON library
void json_escape_append(Nob_String_Builder* sb, const char* str, size_t len) {
	for (size_t i = 0; i < len; i++) {
		uint8_t c = str[i];
		switch (c) {
			case  '"': nob_sb_append_cstr(sb, "\\\""); break;
			case '\\': nob_sb_append_cstr(sb, "\\\\"); break;
			case '\b': nob_sb_append_cstr(sb, "\\b"); break;
			case '\f': nob_sb_append_cstr(sb, "\\f"); break;
			case '\n': nob_sb_append_cstr(sb, "\\n"); break;
			case '\r': nob_sb_append_cstr(sb, "\\r"); break;
			case '\t': nob_sb_append_cstr(sb, "\\t"); break;
			default:
				if (c < 0x20) {
					nob_sb_appendf(sb, "\\u%04x", c);
				} else {
					nob_da_append(sb, c);
				}
		}
	}
}

void HandleAskmeQuestion(struct mg_connection* c, BReader* br) {
	char result = 0;
	uint64_t nanos = nob_nanos_since_unspecified_epoch();
	if (br->count > 256) { nob_return_defer(1); }
	if (br->count == 0) { nob_return_defer(2); }
	if (!ConnCooldown(c)) { nob_return_defer(3); }
	nob_write_entire_file(nob_temp_sprintf("dbs/askme/%lu", nanos), br->data, br->count);
#ifdef TGBOT_ADMIN_CHAT_ID
	Nob_String_Builder sb = {0};
	Nob_String_Builder tmp = {0};
	nob_sb_appendf(&sb, "{\"chat_id\":\""TGBOT_ADMIN_CHAT_ID"\",\"text\":\"");
	nob_sb_append_cstr(&tmp, "Anonymous question: \"");
	nob_da_append_many(&tmp, br->data, br->count);
	nob_sb_append_cstr(&tmp, "\"");
	json_escape_append(&sb, tmp.items, tmp.count);
	nob_sb_appendf(&sb, "\"}");
	TGBotPost(tgb_conn, "sendMessage", "application/json", sb.items, sb.count);
	nob_sb_free(sb);
	nob_sb_free(tmp);
#endif /* TGBOT_ADMIN_CHAT_ID */
	nob_temp_reset();
defer:
	bw_temp.count = 0;
	BWriterAppend(&bw_temp, BU8, SME_ASKME_RESPONCE, BU8, result);
	mg_ws_send(c, bw_temp.items, bw_temp.count, WEBSOCKET_OP_BINARY);
}

void HandleWSMessage(struct mg_connection* c, void* ev_data) {
	struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
	BReader br = {0};
	br.count = wm->data.len;
	br.data = wm->data.buf;
	uint16_t gcmt;
	//mg_hexdump(br.data, br.count);
	if (!BReadU16(&br, &gcmt)) { return; }
	switch (gcmt) {
		case CME_ASKME_QUESTION:
			HandleAskmeQuestion(c, &br);
			break;
	}
}

void EventHandler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_WS_MSG:
			if (!c->fn_data) { c->fn_data = calloc(1, sizeof(ConnData)); }
			NOB_ASSERT(c->fn_data);
			HandleWSMessage(c, ev_data);
			break;
		case MG_EV_HTTP_MSG:
			HandleHTTPMessage(c, ev_data);
			break;
		case MG_EV_POLL:
			VisitorsManageUnactive();
			break;
		case MG_EV_CLOSE:
			free(c->fn_data);
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

#ifdef TGBOT_ENABLE
	 TGBotConnect(&mgr);
#endif /* TGBOT_ENABLE */

	signal(SIGINT, app_terminate);
	signal(SIGTERM, app_terminate);

	int count = 0;
	while (is_working) {
		mg_mgr_poll(&mgr, 1000);
#ifdef TGBOT_ENABLE
		TGBotPoll();
#endif /* TGBOT_ENABLE */
	}

	// Closing
	mg_mgr_free(&mgr);
	printf("Server closed.\n");

	return 0;
}
