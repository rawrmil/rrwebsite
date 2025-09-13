LIB=lib/mongoose.o lib/sds/sds.o
INC=-I./lib
BIN=rrwebsite

all: prebuild main

prebuild:
	gcc -c lib/sds/sds.c -o lib/sds/sds.o
	gcc -c lib/mongoose/mongoose.c -o lib/mongoose.o -DMG_ENABLE_SSI=1

main: ssr_convert
	gcc main.c -o $(BIN) $(LIB) $(INC)

ssr_convert:
	mkdir -p ssr_generated
	gcc ssr_convert.c -o ssr_convert
	./ssr_convert web/index.ssrt.html ssr_generated/ssr_root.h ssr_root

run: main
	./$(BIN)

.PHONY: ssr_convert
