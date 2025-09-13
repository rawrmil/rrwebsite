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

typedef struct SSREntry {
	R_StringBuilder fpath;
	R_StringBuilder fname;
	R_StringBuilder hname;
} SSREntry;

R_DA_DEFINE(SSREntry, SSREntries);

SSREntries ssr_entries = {0};

char AddSSRFile(char* fpath) {
	char tail[] = ".ssrt.html";
	size_t tail_len = strlen(tail);
	size_t fpath_len = strlen(fpath);
	if (fpath_len < tail_len)
		return 0;
	if (strcmp(tail, fpath + fpath_len - tail_len) != 0)
		return 0;
	SSREntry ssr_entry = {0};
	char* ptr = fpath+fpath_len-1;
	for (; ptr != fpath; ptr--) {
		if (*ptr == '/') { ptr++; break; }
	}
	R_SB_APPEND_CSTR(&ssr_entry.fname, ptr);
	R_SB_APPEND_BUFFER(&ssr_entry.fname, "\0", 1);
	R_SB_APPEND_CSTR(&ssr_entry.fpath, fpath);
	R_SB_APPEND_BUFFER(&ssr_entry.fpath, "\0", 1);
	printf("fpath: %s\n", ssr_entry.fpath.buf);
	printf("fname: %s\n", ssr_entry.fname.buf);
	R_DA_APPEND(&ssr_entries, ssr_entry);
	return 1;
}

char ProccessSSRFiles(char* dpath) {
	DIR* dp = opendir(dpath);
	if (!dp) return 0;
	struct dirent* de;
	R_StringBuilder new_dpath;
	while (1) {
		de = readdir(dp);
		if (de == NULL) break;
		if (strcmp(de->d_name, ".") == 0) continue;
		if (strcmp(de->d_name, "..") == 0) continue;
		new_dpath = (R_StringBuilder){0};
		R_SB_AppendFormat(&new_dpath, "%s/%s", dpath, de->d_name);
		R_SB_APPEND_BUFFER(&new_dpath, "\0", 1);
		if (!ProccessSSRFiles(new_dpath.buf))
			AddSSRFile(new_dpath.buf);
		R_SB_FREE(&new_dpath);
	}
cleanup:
	if (dp) closedir(dp);
	return 1;
failure:
	printf("Cannot read a dir '%s'\n", dpath);
	goto cleanup;
}

int ProcessContents(R_StringBuilder sb_in, R_StringBuilder* sb_out_ptr) {
	char cb[] = "<!--%c";
	char ce[] = "%-->";
	char sb[] = "SSR_OUT(\"";
	char se[] = "\");\n";
	R_StringBuilder sb_out = {0};
	char mode = 0;
	size_t line = 0;
	R_SB_AppendFormat(&sb_out, sb);
	for (char* inp = sb_in.buf; *inp; inp++) {
		if (*inp == '\n') line++;
		if (strncmp(inp, ce, strlen(ce)) == 0) {
			inp = inp+strlen(ce);
			assert(mode != 0);
			R_SB_AppendFormat(&sb_out, sb);
			mode = 0;
		}
		if (strncmp(inp, cb, strlen(cb)) == 0) {
			inp = inp+strlen(cb);
			assert(mode != 1);
			R_SB_AppendFormat(&sb_out, se);
			mode = 1;
		}
		if (mode == 0) {
			R_SB_AppendFormat(&sb_out, "\\x%x", *inp);
			continue;
		}
		R_DA_APPEND(&sb_out, *inp);
	}
	R_SB_AppendFormat(&sb_out, se);
	*sb_out_ptr = sb_out;
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("cssr_convert <in_path> <out_path>\n");
		printf("Example: cssr_convert something.cssr.html something_html.c\n");
		return 0;
	}

	// File in
	R_StringBuilder sb_in = {0};
	assert(R_SB_AppendFile(&sb_in, argv[1]));

	// Process
	R_StringBuilder sb_out = {0};
	assert(!ProcessContents(sb_in, &sb_out));

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

	ProccessSSRFiles("./web");
	
	// Cleanup
	R_SB_FREE(&sb_in);
	R_SB_FREE(&sb_out);
}
