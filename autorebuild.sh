#!/bin/sh

ENTR_CMD="make main && ./rrwebsite \"$@\""

find . -type f \
	-name "*.c" -o \
	-name "Makefile" -o \
	-name "*.html" \
	| entr -r bash -c "$ENTR_CMD"
