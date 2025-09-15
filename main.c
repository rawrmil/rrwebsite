#include <stdio.h>
#include <signal.h>
#include <getopt.h>

#include "mongoose/mongoose.h"

// --- SSR ---

struct ssr_data {
	char lang_is_ru;
};

#define RRSTD_IMPLEMENTATION
#include "rrstd.h"
#include "ssr.h"

#define SSR_RU_PRINTF(...) \
	if (ssr_data.lang_is_ru) { SSR_PRINTF(__VA_ARGS__); }
#define SSR_EN_PRINTF(...) \
	if (!ssr_data.lang_is_ru) { SSR_PRINTF(__VA_ARGS__); }
#define SSR_DEREF_DATA(d) \
	struct ssr_data ssr_data = (struct ssr_data*)ssr_user ? *((struct ssr_data*)ssr_user) : (struct ssr_data){0};

#include "ssr_generated/ssr_template_default_before.h"
#include "ssr_generated/ssr_template_default_after.h"
#include "ssr_generated/ssr_root.h"

// --- APP ---

char web_dir_default[] = "./web";

struct a_config {
	int port;
	char* web_dir;
} aconf;

struct a_config a_read_args(int argc, char* argv[]) {
	struct a_config aconf = {0};
	aconf.port = 6969;
	aconf.web_dir = web_dir_default;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"webdir", required_argument, 0, 'd'},
		{0, 0, 0, 0} // NULL-terminator
	};

	int opt;
	errno = 0;
	while ((opt = getopt_long(argc, argv, "hp:d:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'h':
				printf("Help:\n");
				printf("--port, -p - specify running port for the server\n");
				printf("--webdir, -d - specify director for the server to serve\n");
				exit(0);
				break;
			case 'p':
				char* endp;
				aconf.port = strtol(optarg, &endp, 10);
				if (errno || *endp != '\0') {
					printf("--port argument is invalid (0-65535)\n");
					exit(1);
				}
				break;
			case 'd':
				printf("! %s\n", optarg);
				aconf.web_dir = optarg;
				break;
			default:
				break;
		}
	}

	return aconf;
}

// --- EVENTS ---

void ev_handle_http_msg(struct mg_connection* c, void* ev_data) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
	if (!strncmp(hm->method.buf, "GET", 3)) {
		struct ssr_data ssr_data = {0};
		struct mg_str *s = mg_http_get_header(hm, "Accept-Language");
		if (s && s->len >= 2) {
			ssr_data.lang_is_ru = strncmp("ru", s->buf, 2) == 0;
		}
		//mg_http_reply(c, 200, "", "123");
		R_StringBuilder sb = {0};
		R_DA_RESERVE(&sb, 8192);
		//R_SB_APPEND_CSTR(&sb, "123");
		ssr_root(&sb, &ssr_data);
		mg_http_reply(c, 200, "", sb.buf);
		R_SB_FREE(&sb);
		return;
		//struct mg_http_serve_opts opts = { .root_dir = aconf.web_dir, .ssi_pattern="#.shtml" };
		//mg_http_serve_dir(c, hm, &opts);
		//return;
	}
}

void ev_handler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_HTTP_MSG:
			ev_handle_http_msg(c, ev_data);
			break;
	}
}

// --- MAIN ---

char is_working = 1;

void app_terminate(int sig) { is_working = 0; }

int main(int argc, char* argv[]) {
	mg_log_set(MG_LL_NONE);

	aconf = a_read_args(argc, argv);
	printf("! %s\n", aconf.web_dir);

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	char addrstr[32];
	snprintf(addrstr, sizeof(addrstr), "http://0.0.0.0:%d", aconf.port);
	mg_http_listen(&mgr, addrstr, ev_handler, NULL);

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
