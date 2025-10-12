#ifdef SSR_CONVERT_IMPLEMENTATION

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

int ProcessContents(const Nob_String_Builder sb_in, Nob_String_Builder* sb_out) {
	char cbegin[] = "<!--%c";
	char cend[] = "%-->";
	char outbegin[] = "SSR_OUT(\"";
	char outend[] = "\");\n";
	char mode = 0;
	size_t line = 0;
	nob_sb_appendf(sb_out, outbegin);
	for (char* inp = sb_in.items; *inp; inp++) {
		if (*inp == '\n') line++;
		if (strncmp(inp, cend, strlen(cend)) == 0) {
			inp = inp+strlen(cend);
			assert(mode != 0);
			nob_sb_appendf(sb_out, outbegin);
			mode = 0;
		}
		if (strncmp(inp, cbegin, strlen(cbegin)) == 0) {
			inp = inp+strlen(cbegin);
			assert(mode != 1);
			nob_sb_appendf(sb_out, outend);
			mode = 1;
		}
		if (mode == 0) {
			nob_sb_appendf(sb_out, "\\x%02x", (unsigned char)(*inp));
			continue;
		}
		nob_da_append(sb_out, *inp);
	}
	nob_sb_appendf(sb_out, outend);
	return 0;
}

int SSRConvert(char* in_path, char* out_path, char* func_name) {
	printf("SSRConvert: %s -> %s : %s\n", in_path, out_path, func_name);
	// File in
	Nob_String_Builder sb_in = {0};
	assert(R_SB_AppendFile(&sb_in, in_path));
	nob_sb_append_null(&sb_in);

	// Process
	Nob_String_Builder sb_out = {0};
	nob_sb_appendf(&sb_out, "static inline void %s(Nob_String_Builder* ssr_sb, SSRData ssr_data);\n", func_name);
	nob_sb_appendf(&sb_out, "static inline void %s(Nob_String_Builder* ssr_sb, SSRData ssr_data) {\n", func_name);
	assert(!ProcessContents(sb_in, &sb_out));
	nob_sb_appendf(&sb_out, "}\n");

	// File out
	FILE *out_fp = fopen(out_path, "w");
	if (out_fp == NULL) {
		printf("Cannot open a dst file '%s'\n", func_name);
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

#endif /* SSR_CONVERT_IMPLEMENTATION */
