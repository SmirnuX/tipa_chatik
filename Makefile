objects = src/main.c src/client.c src/server.c src/ui.c
output = tchat

all:
	gcc $(objects) -o $(output)
	
sanitized: 
	gcc $(objects) -o $(output) -fsanitize=address
