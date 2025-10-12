#ifndef SPLASHES_H
#define SPLASHES_H

const char* splashes[][2] = {
	{ "This page was made with HTML, CSS, C only", "Эта страница была создана только с помощью HTML, CSS, C" },
	{ "Move like old man does", "Двигайся как старик" },
	{ "BNPL, you own nothing anyway", "BNPL, ты всё равно не владеешь ничем" }
};

#define SPLASHES_LEN() sizeof(splashes)/sizeof(*splashes)

#define SPLASHES_GET(lang_ru_) splashes[rand()%SPLASHES_LEN()][lang_ru_]

#endif /* SPLASHES_H */
