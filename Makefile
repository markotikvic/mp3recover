all:
	gcc main.c util.c -Wall -o mp3recover -lid3tag -std=c99
