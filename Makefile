objects = src/main.c src/client.c src/server.c src/ui.c
output = tchat

all:
	gcc $(objects) -o $(output)
	
sanitized: 
	cppcheck --enable=all --inconclusive --std=posix ./src/*
	gcc $(objects) -o $(output) -fsanitize=address

warnings:
	gcc $(objects) -o $(output) -O2 -Wall -Wextra -Wno-sign-compare
