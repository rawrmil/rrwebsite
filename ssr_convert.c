#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <dirent.h>

#define RRSTD_IMPLEMENTATION
#include "rrstd.h"

char R_SB_AppendFile(R_StringBuilder* sb, char* fpath) {
	FILE *fp = fopen(fpath, "rb");
	if (fp == NULL) goto failure;

	fseek(fp, 0, SEEK_END);
	size_t new_len = sb->len + ftell(fp);
	fseek(fp, 0, SEEK_SET);

	R_DA_RESERVE(sb, new_len);
	size_t read = fread(sb->buf + sb->len, 1, new_len, fp);
	if (ferror(fp)) goto failure;
	sb->len = read;

cleanup:
	if (fp) fclose(fp);
	return 1;
failure:
	printf("Cannot read a file '%s'\n", fpath);
	goto cleanup;
}

int ProcessContents(R_StringBuilder sb_in, R_StringBuilder* sb_out) {
	char cb[] = "<!--%c";
	char ce[] = "%-->";
	char sb[] = "SSR_OUT(\"";
	char se[] = "\");\n";
	char mode = 0;
	size_t line = 0;
	R_SB_AppendFormat(sb_out, sb);
	for (char* inp = sb_in.buf; *inp; inp++) {
		if (*inp == '\n') line++;
		if (strncmp(inp, ce, strlen(ce)) == 0) {
			inp = inp+strlen(ce);
			assert(mode != 0);
			R_SB_AppendFormat(sb_out, sb);
			mode = 0;
		}
		if (strncmp(inp, cb, strlen(cb)) == 0) {
			inp = inp+strlen(cb);
			assert(mode != 1);
			R_SB_AppendFormat(sb_out, se);
			mode = 1;
		}
		if (mode == 0) {
			R_SB_AppendFormat(sb_out, "\\x%x", *inp);
			continue;
		}
		R_DA_APPEND(sb_out, *inp);
	}
	R_SB_AppendFormat(sb_out, se);
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("cssr_convert <in_path> <out_path> <function_name>\n");
		printf("Example: cssr_convert something.cssr.html something_html.c\n");
		return 0;
	}

	// File in
	R_StringBuilder sb_in = {0};
	assert(R_SB_AppendFile(&sb_in, argv[1]));

	// Process
	R_StringBuilder sb_out = {0};
	R_SB_AppendFormat(&sb_out, "R_StringBuilder %s() {\n", argv[3]);
	R_SB_AppendFormat(&sb_out, "\tR_StringBuilder ssr_sb = {0};\n");
	assert(!ProcessContents(sb_in, &sb_out));
	R_SB_AppendFormat(&sb_out, "\treturn ssr_sb;\n");
	R_SB_AppendFormat(&sb_out, "}\n");

	// File out
	FILE *out_fp = fopen(argv[2], "w");
	if (out_fp == NULL) {
		printf("Cannot open a dst file '%s'\n", argv[2]);
		return 1;
	}
	size_t written = fwrite(sb_out.buf, 1, sb_out.len, out_fp);
	if (written != sb_out.len) {
		printf("Error writing a file\n");
		return 1;
	}
	fclose(out_fp);
	
	// Cleanup
	R_SB_FREE(&sb_in);
	R_SB_FREE(&sb_out);
}
