#define NOB_STRIP_PREFIX

#define SSR_CONVERT_IMPLEMENTATION
#include "ssr_convert.h"

/* Always last */
#define NOB_IMPLEMENTATION
#include "nob.h"

#define CC "gcc"

int main(int argc, char** argv) {

	NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "ssr_convert.h");
	
	Cmd cmd = {0};

	if (needs_rebuild1("lib/mongoose.o", "lib/mongoose/mongoose.c")) {
		cmd_append(&cmd, CC, "-c", "lib/mongoose/mongoose.c", "-o", "lib/mongoose.o");
		if (!cmd_run(&cmd)) return 0;
	}

	//if (needs_rebuild1("ssr_convert", "ssr_convert.c")) {
	//	cmd_append(&cmd, CC, "ssr_convert.c", "-o", "ssr_convert");
	//	if (!cmd_run(&cmd)) return 0;
	//}

	SSRConvert("web/index.html", "ssr_generated/ssr_root.h", "ssr_root");
	SSRConvert("web/about.html", "ssr_generated/ssr_about.h", "ssr_about");
	SSRConvert("web/page404.html", "ssr_generated/ssr_page404.h", "ssr_page404");
	SSRConvert("web/proj/mna.html", "ssr_generated/ssr_proj_mna.h", "ssr_proj_mna");
	SSRConvert("web/templates/default-after.html", "ssr_generated/ssr_template_default_after.h", "ssr_template_default_after");
	SSRConvert("web/templates/default-before.html", "ssr_generated/ssr_template_default_before.h", "ssr_template_default_before");
	
	cmd_append(&cmd, CC, "main.c", "-o", "rrwebsite");
	cmd_append(&cmd, "lib/mongoose.o");
	cmd_append(&cmd, "-I./lib");
	if (!cmd_run(&cmd)) return 0;

	return 0;
}
