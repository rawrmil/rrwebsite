LIB=lib/mongoose.o
INC=-I./lib
BIN=rrwebsite

all: mongoose main

mongoose:
	gcc -c lib/mongoose/mongoose.c -o lib/mongoose.o -DMG_ENABLE_SSI=1

main:
	gcc main.c -o $(BIN) $(LIB) $(INC)

run: main
	./$(BIN)
