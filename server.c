// SPDX-License-Identifier: CPOL-1.02
#include "main.h"



int server(int server_socket, char* nickname)
{
    room_count = 0;
	for (int i=0; i<MAXROOMS; i++)	//TODO - проверить целесообразность выделения памяти
		rooms[i] = malloc(sizeof(char) * MAXNICKLEN);	//TODO - освободить память
    DIR* directory = opendir("server_history");
	if (directory==NULL)
	{
		printf("Ошибка открытия директории \"server_history\"");
		return 2;
	}
	chdir("server_history");	//Переходим в эту директорию
	struct dirent* dir_ptr;
	struct stat stat_file;
	dir_ptr=readdir(directory);
	while (dir_ptr!=NULL)	//Открытие файлов, в которых хранится истории сообщений
	{
		if ((*dir_ptr).d_name[0] != '.')	//Пропуск скрытых файлов
		{
			
			lstat((*dir_ptr).d_name, &stat_file);	//Проверяем на то, является ли файл папкой
			if (!S_ISDIR(stat_file.st_mode))	//Записываем имена комнат в массив
			{	
				strncpy(rooms[room_count],(*dir_ptr).d_name, MAXNICKLEN);
				room_fd[room_count] = open(rooms[room_count], O_RDWR);
				room_number[room_count] = read_messages(room_fd[0]);
				printf("%i.%s (%li байт, %i сообщений)\n", room_count+1,rooms[room_count], lseek(room_fd[room_count], 0, SEEK_END), room_number[room_count]);
				room_count++;
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
		char buf[MAXBUFFER];
		//Получение команды
		get_message(client_socket, buf);
		printf("Принято %s", buf);
		
		//Идентификация команды
		for (int i = 0; i < CMD_COUNT; i++)	
		{
			if (strcmp(buf, server_cmd_strings[i]) == 0)	
			{
				printf("Выполняется %s", buf);
				server_cmd_functions[i](client_socket, NULL);
				break;
			}
		}
        //get_rooms_server(client_socket, NULL);
		//send_message_server(client_socket, NULL);
    }
    return 0;
}

int get_rooms_server(int sock, char** args)	//Отправка списка комнат через сокет sock, аргумент args не используется
{
	char buf[MAXBUFFER];
	snprintf(buf, MAXBUFFER, "%i", room_count);	//Отправка количества комнат
	send_message(sock, buf);
	for (int i=0; i<room_count; i++)	
	{
		strncpy(buf, rooms[i], MAXNICKLEN);
		send_message(sock, buf);	//Отправка названий комнат
	}
}

int send_message_server(int sock, char** args)	//Принятие сообщения сервером
{
	char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	get_message(sock, buf);	//Получение номера комнаты 
	int room = atoi(buf);
	//Время получения сообщения сервером
	time_t timer = time(NULL);
	strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
	printf(DIM WHITE"%s\n", s_time);
	//Никнейм отправителя
	get_message(sock, nickname);
	printf(DEFAULT BRIGHT" %s\n", nickname);
	//Сообщение
	get_message(sock, buf);
	printf(DEFAULT"%s\n\n", buf);
	write_message(room_fd[room], s_time, nickname, buf, ++room_number[room]);
}

int get_new_messages_server(int sock, char** args)	//Отправка недостаюших сообщений клиенту. Аргумент args не используется
{
    char buf[MAXBUFFER];
    //Получение комнаты
	int room = atoi(get_message(sock, buf));
    //Получение количества сообщений
    int count = atoi(get_message(sock, buf));	
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", room_number[room]);
    send_message(sock, buf);
	//Перемещение к первому передаваемому сообщению
	goto_message(room_fd[room], room_number[room] - count);	//TODO - вставить проверку на room_number < count
	struct message msg;
	//printf("%i|", room_number[room]-count);
	for (int i = 0; i < room_number[room]-count ; i++)
	{	
		//printf("%i ", i);
		//Считывание сообщения
		msg.datetime = "12.02.12";
		msg.nickname = "test";
		msg.msg_text = "hello world";
    	//Передача сообщений
		strncpy(buf, msg.msg_text, strlen(msg.msg_text)+1);
		send_message(sock, buf);
		send_message(sock, buf);
		printf("before %i, ", send_message(sock, buf));
	}
	return 1;	
}


