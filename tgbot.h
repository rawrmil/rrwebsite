#ifdef TELEGRAM_API_TOKEN

#ifndef TGBOT_RW_H
#define TGBOT_RW_H

#define TELEGRAM_API_HOST "api.telegram.org"
#define TELEGRAM_API_URL "https://"TELEGRAM_API_HOST"/"

void TGBotEventHandler(struct mg_connection* c, int ev, void* ev_data);
void TGBotConnect(struct mg_mgr* mgr);

#endif /* TGBOT_RW_H */

#ifdef TGBOT_IMPLEMENTATION

void TGBotConnect(struct mg_mgr* mgr) {
	mg_http_connect(mgr, TELEGRAM_API_URL, TGBotEventHandler, NULL);
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
			if (mg_log_level >= MG_LL_INFO) {
				mg_printf(c,
						"GET /bot"TELEGRAM_API_TOKEN"/getMe HTTP/1.0\r\n"
						"Host: "TELEGRAM_API_HOST"\r\n\r\n");
			}
			break;
		case MG_EV_HTTP_MSG:
			MG_INFO(("TGBOT: MSG\n"));
			struct mg_http_message* hm = (struct mg_http_message*)ev_data;
			mg_hexdump(hm->body.buf, hm->body.len);
			break;
		case MG_EV_ERROR:
			MG_INFO(("TGBOT: ERROR '%s'\n", ev_data));
			break;
	}
}

#endif /* TGBOT_IMPLEMENTATION */
#endif /* TELEGRAM_API_TOKEN */
