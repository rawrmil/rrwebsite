#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define CC "gcc"

#define SSR_CONVERT(src_, dst_, func_) \
	do { \
		cmd_append(&cmd, "./ssr_convert", src_, dst_, func_); \
		if (!cmd_run(&cmd)) return 0; \
	} while(0);

int main(int argc, char** argv) {

	NOB_GO_REBUILD_URSELF(argc, argv);
	
	Cmd cmd = {0};

	if (needs_rebuild1("lib/mongoose.o", "lib/mongoose/mongoose.c")) {
		cmd_append(&cmd, CC, "-c", "lib/mongoose/mongoose.c", "-o", "lib/mongoose.o");
		if (!cmd_run(&cmd)) return 0;
	}

	if (needs_rebuild1("ssr_convert", "ssr_convert.c")) {
		cmd_append(&cmd, CC, "ssr_convert.c", "-o", "ssr_convert");
		if (!cmd_run(&cmd)) return 0;
	}

	SSR_CONVERT("web/index.html", "ssr_generated/ssr_root.h", "ssr_root");
	SSR_CONVERT("web/page404.html", "ssr_generated/ssr_page404.h", "ssr_page404");
	SSR_CONVERT("web/templates/default-after.html", "ssr_generated/ssr_template_default_after.h", "ssr_template_default_after");
	SSR_CONVERT("web/templates/default-before.html", "ssr_generated/ssr_template_default_before.h", "ssr_template_default_before");

	
	cmd_append(&cmd, CC, "main.c", "-o", "rrwebsite");
	cmd_append(&cmd, "lib/mongoose.o");
	cmd_append(&cmd, "-I./lib");
	if (!cmd_run(&cmd)) return 0;

	return 0;
}
