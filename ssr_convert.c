#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <dirent.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

char R_SB_AppendFile(Nob_String_Builder* sb, char* fpath) {
	FILE *fp = fopen(fpath, "rb");
	if (fp == NULL) goto failure;

	fseek(fp, 0, SEEK_END);
	size_t new_len = sb->count + ftell(fp);
	fseek(fp, 0, SEEK_SET);

	nob_da_reserve(sb, new_len);
	size_t read = fread(sb->items + sb->count, 1, new_len, fp);
	if (ferror(fp)) goto failure;
	sb->count = read;

cleanup:
	if (fp) fclose(fp);
	return 1;
failure:
	printf("Cannot read a file '%s'\n", fpath);
	goto cleanup;
}

int ProcessContents(Nob_String_Builder sb_in, Nob_String_Builder* sb_out) {
	char cb[] = "<!--%c";
	char ce[] = "%-->";
	char sb[] = "SSR_OUT(\"";
	char se[] = "\");\n";
	char mode = 0;
	size_t line = 0;
	nob_sb_appendf(sb_out, sb);
	for (char* inp = sb_in.items; *inp; inp++) {
		if (*inp == '\n') line++;
		if (strncmp(inp, ce, strlen(ce)) == 0) {
			inp = inp+strlen(ce);
			assert(mode != 0);
			nob_sb_appendf(sb_out, sb);
			mode = 0;
		}
		if (strncmp(inp, cb, strlen(cb)) == 0) {
			inp = inp+strlen(cb);
			assert(mode != 1);
			nob_sb_appendf(sb_out, se);
			mode = 1;
		}
		if (mode == 0) {
			nob_sb_appendf(sb_out, "\\x%02x", (unsigned char)(*inp));
			continue;
		}
		nob_da_append(sb_out, *inp);
	}
	nob_sb_appendf(sb_out, se);
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("cssr_convert <in_path> <out_path> <function_name>\n");
		printf("Example: cssr_convert something.cssr.html something_html.c\n");
		return 0;
	}

	// File in
	Nob_String_Builder sb_in = {0};
	assert(R_SB_AppendFile(&sb_in, argv[1]));

	// Process
	Nob_String_Builder sb_out = {0};
	nob_sb_appendf(&sb_out, "static inline void %s(Nob_String_Builder* ssr_sb, SSRData ssr_data);\n", argv[3]);
	nob_sb_appendf(&sb_out, "static inline void %s(Nob_String_Builder* ssr_sb, SSRData ssr_data) {\n", argv[3]);
	assert(!ProcessContents(sb_in, &sb_out));
	nob_sb_appendf(&sb_out, "}\n");

	// File out
	FILE *out_fp = fopen(argv[2], "w");
	if (out_fp == NULL) {
		printf("Cannot open a dst file '%s'\n", argv[2]);
		return 1;
	}
	size_t written = fwrite(sb_out.items, 1, sb_out.count, out_fp);
	if (written != sb_out.count) {
		printf("Error writing a file\n");
		return 1;
	}
	fclose(out_fp);
	
	// Cleanup
	nob_sb_free(sb_in);
	nob_sb_free(sb_out);
}
