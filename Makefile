#!/bin/sh
all:
	gcc -O2 -Wall -o prettynames renamer.c
	strip ~/bin/prettynames 
