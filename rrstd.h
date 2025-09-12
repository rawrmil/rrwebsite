// Standart library alternative
#ifndef RRSTD_H
#define RRSTD_H

// A lot of code taken from
// https://github.com/tsoding/nob.h
// like... A LOT

#include <stdarg.h>

#ifndef R_ASSERT
#include <assert.h>
#define R_ASSERT assert
#endif /* R_ASSERT */

#ifndef R_REALLOC
#include <stdlib.h>
#define R_REALLOC realloc
#endif /* R_REALLOC */

#ifndef R_FREE
#include <stdlib.h>
#define R_FREE free
#endif /* R_FREE */

#ifdef __cplusplus
#define R_DECLTYPE_CAST(T) (decltype(T))
#else
#define R_DECLTYPE_CAST(T)
#endif // __cplusplus

// --- Dynamic Arrays ---

#define R_DA_DEFINE(ITEM_TYPE_, DA_TYPE_) \
	typedef struct { \
		ITEM_TYPE_* buf; \
		size_t len; \
		size_t cap; \
	}	DA_TYPE_;

#define R_DA_RESERVE(DA_, EXP_CAP_) \
    do { \
        if ((EXP_CAP_) > (DA_)->cap) { \
            if ((DA_)->cap == 0) { (DA_)->cap = 16; } \
            while ((EXP_CAP_) > (DA_)->cap) { (DA_)->cap *= 2; } \
            (DA_)->buf = R_DECLTYPE_CAST((DA_)->buf)R_REALLOC((DA_)->buf, (DA_)->cap*sizeof(*(DA_)->buf)); \
            R_ASSERT((DA_)->buf != NULL && "Buy more RAM lol"); \
        } \
    } while (0)

#define R_DA_APPEND(DA_, ITEM_) \
    do { \
        R_DA_RESERVE((DA_), (DA_)->len + 1); \
        (DA_)->buf[(DA_)->len++] = (ITEM_); \
    } while (0)

#define R_DA_FREE(DA_) R_FREE((DA_)->buf)

#define R_DA_APPEND_MANY(DA_, ITEMS_, ITEMS_LEN_) \
    do { \
        R_DA_RESERVE((DA_), (DA_)->len + (ITEMS_LEN_)); \
        memcpy((DA_)->buf + (DA_)->len, (ITEMS_), (ITEMS_LEN_)*sizeof(*(DA_)->buf)); \
        (DA_)->len += (ITEMS_LEN_); \
    } while (0)

#define R_DA_RESIZE(DA_, NEW_SIZE_) \
    do { \
        R_DA_RESERVE((DA_), NEW_SIZE_); \
        (DA_)->len = (NEW_SIZE_); \
    } while (0)

#define R_DA_LAST(DA_) (DA_)->buf[(R_ASSERT((DA_)->len > 0), (DA_)->len-1)]
#define R_DA_REMOVE_INDEX(DA_, I_) \
    do { \
        size_t j_ = (I_); \
        R_ASSERT(j_ < (DA_)->len); \
        (DA_)->buf[j_] = (DA_)->buf[--(DA_)->len]; \
    } while(0)

#define R_DA_FOREACH(TYPE_, IT_, DA_) \
	for (TYPE_ *IT_ = (DA_)->buf; IT_ < (DA_)->buf + (DA_)->len; ++IT_)

// --- String Builder ---

R_DA_DEFINE(char, R_StringBuilder);

#define R_SB_APPEND_CSTR(SB_, BUF_) R_DA_APPEND_MANY((SB_), BUF_, strlen(BUF_))
#define R_SB_APPEND_BUFFER(SB_, BUF_, LEN_) R_DA_APPEND_MANY((SB_), BUF_, LEN_)
int R_SB_AppendFormat(R_StringBuilder *sb, const char *fmt, ...);

#define R_SB_FREE(SB_) R_DA_FREE((SB_))

// --- Implementation ---

#ifdef RRSTD_IMPLEMENTATION

// TODO: NOBDEF from nob.h

int R_SB_AppendFormat(R_StringBuilder *sb, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	R_DA_RESERVE(sb, sb->len + n + 1); // With NULL-terminator

	char* dst = sb->buf + sb->len;

	va_start(args, fmt);
	vsnprintf(dst, n+1, fmt, args);
	va_end(args);

	sb->len += n; // Without NULL-terminator

	return n;
}

#endif

#endif /* RRSTD_H */
