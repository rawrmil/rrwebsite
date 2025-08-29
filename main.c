#include <stdio.h>
#include <signal.h>
#include <getopt.h>

#include "mongoose/mongoose.h"

// --- APP ---

struct a_config {
	int port;
};

struct a_config a_read_args(int argc, char* argv[]) {
	struct a_config aconf = {0};
	aconf.port = 6969;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0} // NULL-terminator
	};

	int opt;
	errno = 0;
	while ((opt = getopt_long(argc, argv, "hp:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'h':
				printf("Help:\n");
				printf("--port, -p - specify running port for the server\n");
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
		struct mg_http_serve_opts opts = { .root_dir = "./web", .ssi_pattern="#.shtml" };
		mg_http_serve_dir(c, hm, &opts);
		return;
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

	struct a_config aconf = a_read_args(argc, argv);

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
