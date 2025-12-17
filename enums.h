#ifndef ENUMS_H
#define ENUMS_H

#include "nob.h"

// --- Client Message Enum (CME) ---

#define CME \
	X(CME_ASKME_QUESTION) \
	X(CME_LENGTH) \

#define X(name_) name_,
enum { CME };
#undef X

// --- Server Message Enum (SME) ---

#define SME \
	X(SME_LENGTH) \

#define X(name_) name_,
enum { SME };
#undef X

// --- Names ---

extern const char *CME_NAMES[];
extern const char *SME_NAMES[];

// --- JS ---

extern Nob_String_Builder enums_js;

void GameMessageTypesGenerateJS();

#endif /* ENUMS_H */

#ifdef ENUMS_IMPLEMENTATION

#define X(name_) #name_,
const char *CME_NAMES[] = { CME };
#undef X

#define X(name_) #name_,
const char *SME_NAMES[] = { SME };
#undef X

Nob_String_Builder enums_js;

#define JS_ENUM_GENERATE(name_) \
	do { \
		nob_sb_append_cstr(&enums_js, "const "#name_" = {\n"); \
		for (int i = 0; i < name_##_LENGTH; i++) { \
			nob_sb_appendf(&enums_js, "\t%s: %d,\n", name_##_NAMES[i] + strlen(#name_"_"), i); \
		} \
		nob_sb_append_cstr(&enums_js, "};\n"); \
	} while(0);

void EnumsGenerateJS() {
	JS_ENUM_GENERATE(CME);
	JS_ENUM_GENERATE(SME);
	//nob_sb_appendf(&enums_js, "const debug = %s;\n", game.debug ? "true" : "false"); // TODO: by flag
	nob_sb_append_null(&enums_js);
}

#endif /* ENUMS_IMPLEMENTATION */
