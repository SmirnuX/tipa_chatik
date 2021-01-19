// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int server(int server_socket, char* nickname)
{
    //Проверка истории сообщений
    int count = 0;
    char* rooms[MAXROOMS];
	for (int i=0; i<MAXROOMS; i++)
		rooms[i] = malloc(sizeof(char) * MAXNICKLEN);
    DIR* directory = opendir("server_history");
	if (directory==NULL)
	{
		printf("Ошибка открытия директории server_history");
		return 2;
	}
	chdir("server_history");	//Переходим в эту директорию
	struct dirent* dir_ptr;
	struct stat stat_file;
	dir_ptr=readdir(directory);
	while (dir_ptr!=NULL)
	{
		if ((*dir_ptr).d_name[0] != '.')	//Пропуск скрытых файлов
		{
			//Проверяем на то, является ли файл папкой
			lstat((*dir_ptr).d_name, &stat_file);
			
			if (!S_ISDIR(stat_file.st_mode))
			{	
				strncpy(rooms[count],(*dir_ptr).d_name, MAXNICKLEN);
				printf("%i.%s\n",count+1,rooms[count]);
				count++;
			}
		}
		dir_ptr=readdir(directory);	//Переходим к следующему файлу
	}
	closedir(directory);

    
    listen(server_socket, 5);
    while (1)
    {
        int client_socket;
        struct sockaddr_in client_address;
        printf("Ожидание...\n");
        int len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*) &client_address, &len);
        
        get_rooms_server(client_socket, count, rooms);
		send_message_server(client_socket);
		close(client_socket);
    }
    return 0;
}

void get_rooms_server(int sock, int count, char** rooms)
{
	char buf[MAXBUFFER];
	get_message(sock, buf);
    printf("%s\n", buf);
	snprintf(buf, MAXBUFFER, "%i", count);
	send_message(sock, buf);
	for (int i=0; i<count; i++)
	{
		strncpy(buf, rooms[i], MAXNICKLEN);
		send_message(sock, buf);
	}
}

int send_message_server(int sock)
{
	char buf[MAXBUFFER];
	get_message(sock, buf);
	printf("%s\n", buf);
	get_message(sock, buf);
	printf(BLUE"%s\t", buf);
	time_t timer = time(NULL);
	strftime(buf, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
	printf(DIM WHITE"%s\n", buf);
	get_message(sock, buf);
	printf(DEFAULT"%s\n\n", buf);
}
