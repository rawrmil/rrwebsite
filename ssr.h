#define SSR_OUT(CSTR_) R_SB_AppendFormat(&ssr_sb, "%s", CSTR_);
#define SSR_PRINTF(...) R_SB_AppendFormat(&ssr_sb, __VA_ARGS__);
