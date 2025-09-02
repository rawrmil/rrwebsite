LIB=lib/mongoose.o lib/sds/sds.o
INC=-I./lib
BIN=rrwebsite

all: prebuild main

prebuild:
	gcc -c lib/sds/sds.c -o lib/sds/sds.o
	gcc -c lib/mongoose/mongoose.c -o lib/mongoose.o -DMG_ENABLE_SSI=1

main:
	gcc main.c -o $(BIN) $(LIB) $(INC)

run: main
	./$(BIN)
