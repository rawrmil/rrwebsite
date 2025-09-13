#!/bin/sh

find . -type f \
	-name "*.c" -o \
	-name "Makefile" -o \
	-name "*.ssrt.*" \
	| entr -r make run
