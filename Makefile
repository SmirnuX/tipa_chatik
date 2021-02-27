objects = src/main.c src/client.c src/server.c src/ui.c
output = tchat

all:
	cppcheck --enable=all --inconclusive --std=posix ./src/*
	gcc $(objects) -o $(output)
	
sanitized: 
	cppcheck --enable=all --inconclusive --std=posix ./src/*
	gcc $(objects) -o $(output) -fsanitize=address
