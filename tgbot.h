#ifdef TGBOT_ENABLE

#ifndef TGBOT_RW_H
#define TGBOT_RW_H

#define TGBOT_API_HOST "api.telegram.org"
#define TGBOT_API_URL "https://"TGBOT_API_HOST"/"

void TGBotEventHandler(struct mg_connection* c, int ev, void* ev_data);
struct mg_connection* TGBotConnect(struct mg_mgr* mgr);
void TGBotPoll(struct mg_connection* c);

extern uint64_t tgb_last_poll_ms;

#endif /* TGBOT_RW_H */

#ifdef TGBOT_IMPLEMENTATION

uint64_t tgb_last_poll_ms;

struct mg_connection* TGBotConnect(struct mg_mgr* mgr) {
	return mg_http_connect(mgr, TGBOT_API_URL, TGBotEventHandler, NULL);
}

void TGBotRequest(struct mg_connection* c, char* method) {
	mg_printf(c,
			"GET /bot"TGBOT_API_TOKEN"/%s HTTP/1.1\r\n"
			"Host: "TGBOT_API_HOST"\r\n"
			"Connection: keep-alive\r\n"
			"\r\n", method);
}

void TGBotPoll(struct mg_connection* c) {
	uint64_t now = mg_millis();
	if (now - tgb_last_poll_ms > 2000) {
		MG_INFO(("TGBOT: POLL\n"));
		TGBotRequest(c, "getUpdates");
		tgb_last_poll_ms = now;
	}
}

void TGBotEventHandler(struct mg_connection* c, int ev, void* ev_data) {
	switch (ev) {
		case MG_EV_CONNECT:
			MG_INFO(("TGBOT: CONNECTION\n"));
			struct mg_str ca = mg_file_read(&mg_fs_posix, "/etc/ssl/certs/ca-certificates.pem");
			struct mg_tls_opts opts = { .ca=ca, .name=mg_str("telegram.org") };
			mg_tls_init(c, &opts);
			break;
		case MG_EV_TLS_HS:
			MG_INFO(("TGBOT: HANDSHAKE\n"));
			TGBotRequest(c, "getMe");
			break;
		case MG_EV_HTTP_MSG:
			MG_INFO(("TGBOT: MSG\n"));
			struct mg_http_message* hm = (struct mg_http_message*)ev_data;
			//mg_hexdump(hm->body.buf, hm->body.len);
			break;
		case MG_EV_ERROR:
			MG_INFO(("TGBOT: ERROR '%s'\n", ev_data));
			break;
	}
}

#endif /* TGBOT_IMPLEMENTATION */
#endif /* TGBOT_ENABLE */
