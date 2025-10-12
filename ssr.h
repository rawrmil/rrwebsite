#define SSR_OUT(CSTR_) nob_sb_appendf(ssr_sb, "%s", CSTR_);
#define SSR_PRINTF(...) nob_sb_appendf(ssr_sb, __VA_ARGS__);
