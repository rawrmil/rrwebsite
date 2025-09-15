#!/bin/sh

find . -type f \
	-name "*.c" -o \
	-name "Makefile" -o \
	-name "*.html" \
	| entr -r make run
