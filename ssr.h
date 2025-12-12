#ifndef SSR_H
#define SSR_H

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

#define SSR_MATCH(URI_, FUNC_) \
	if (URICompare(hm->uri, mg_str(URI_))) return FUNC_;

#endif /* SSR_H */

#ifdef SSR_IMPLEMENTATION

#endif /* SSR_IMPLEMENTATION */
