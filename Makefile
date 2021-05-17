objects = src/main.c src/client.c src/server.c src/ui.c src/functions.c src/autoupdate.c
output = tchat

all:
	gcc $(objects) -o $(output)
	
sanitized: 
	cppcheck --enable=all --inconclusive --std=posix ./src/*
	gcc $(objects) -o $(output) -fsanitize=address
