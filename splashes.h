#ifndef SPLASHES_H
#define SPLASHES_H

const char* splashes[][2] = {
	{
		"This page was made with HTML, CSS, C only",
		"Эта страница была создана только с помощью HTML, CSS, C"
	},
	{
		"Old man moves smart",
		"Old man moves smart"
	},
	{
		"BNPL, you own nothing anyway",
		"BNPL, you own nothing anyway"
	},
	{
		"Art is harmful",
		"Искусство вредно"
	},
	{
		"Hey, officer fiddlesticks, there is a crime in progress",
		"Hey, officer fiddlesticks, there is a crime in progress"
	},
	{
		"B__________a crush",
		"Б__________а краш"
	},
	{
		"Isn't it strange... watching people... try and outrun the rain",
		"Isn't it strange... watching people... try and outrun the rain"
	},
	{
		"\"You have nothing to hide anyway\"",
		"\"You have nothing to hide anyway\""
	},
};

#define SPLASHES_LEN() sizeof(splashes)/sizeof(*splashes)

#define SPLASHES_GET(lang_ru_) splashes[rand()%SPLASHES_LEN()][lang_ru_]
//#define SPLASHES_GET(lang_ru_) splashes[SPLASHES_LEN()-1][lang_ru_]

#endif /* SPLASHES_H */
