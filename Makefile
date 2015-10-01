#Makefile - dib

CC=gcc

dib: dib.c
	$(CC) -o dib dib.c -ltsk

