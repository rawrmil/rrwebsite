#!/bin/sh

find . | grep -e '*.c' -e 'Makefile' | entr -r make run
