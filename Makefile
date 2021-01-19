objects = main.c client.c server.c
output = tchat

all:
	gcc $(objects) -o $(output)
	
sanitized: 
	gcc $(objects) -o $(output) -fsanitize=address
