#!/bin/sh

ENTR_CMD="./nob && ./rrwebsite $@"

find . -type f \
	-name "*.c" -o \
	-name "Makefile" -o \
	-name "*.html" \
	| entr -r bash -c "$ENTR_CMD"
