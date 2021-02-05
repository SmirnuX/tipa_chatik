// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int server(struct s_connection* connection)
{
    room_count = 0;
	for (int i=0; i<MAXROOMS; i++)
		rooms[i] = NULL;
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
				if (rooms[room_count] == NULL)
					rooms[room_count] = malloc(sizeof(char) * MAXNICKLEN);
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

	//Подготовка списка соединений
	int client_connection;	//Сокет соединения
	fd_set client_connections_set;	//Набор файловых дескрипторов соединений
	int client_connection_active = 0;	//Активно ли соединение
	FD_ZERO(&client_connections_set);	//Обнуление набора
	struct timeval timeout;
	
    listen(connection->sock, MAXQUEUE);
	printf("Ожидание...\n");
    while (1)
    {
		if (client_connection_active != 0)
		{
			errno = 0;
			timeout.tv_usec = TIMEOUT_MS;
			timeout.tv_sec = TIMEOUT_S;
			select(client_connection+1, &client_connections_set, NULL, NULL, &timeout);	
			printf("Прошло %li с %li мс\n", timeout.tv_sec, timeout.tv_usec);
			
			//Проверка, можно ли еще считать
			int n = 0;
			ioctl(client_connection, FIONREAD, &n);	//Считываем количество бит, которые можно прочитать
			//int end_of_read = FD_ISSET(connections[i], &connections_set) && !(n==0);
			int end_of_read = !(n==0);
			if (end_of_read)	 
			{
				//Выполнение команды
				char buf[MAXBUFFER];
				//Получение команды
				get_message(client_connection, buf);
				if (buf[0] == '\0')
					end_of_read = 0;
				else
				{
					printf("Принято %s\n", buf);
					//Идентификация команды
					for (int j = 0; j < CMD_COUNT; j++)	
					{
						if (strcmp(buf, server_cmd_strings[j]) == 0)	
						{
							printf("Выполняется %s\n", buf);
							server_cmd_functions[j](client_connection);
							break;
						}
					}
				}
			}
			if (!end_of_read)	//Закрытие неактивных соединений
			{
				printf("Закрытие %i\n", client_connection);
				FD_CLR(client_connection, &client_connections_set);
				close(client_connection);
				client_connection_active = 0;
			}
		}
		if (client_connection_active == 0)	//Добавление соединения из очереди			
		{	
			errno = 0;
			int client_socket;
			struct sockaddr_in client_address;
			int len = sizeof(client_address);
			client_socket = accept(connection->sock, (struct sockaddr*) &client_address, &len);
			if (errno == EAGAIN)
				continue;
			else
			{
				//Добавление соединения в набор
				FD_SET(client_socket, &client_connections_set);
				client_connection = client_socket;
				printf("Добавлено новое соединение\n");
				client_connection_active = 1;
			}
			printf("Ожидание...\n");
		}
    }
	//Освобождение памяти
	for (int i=0; i<MAXROOMS; i++)	
		if (rooms[i] != NULL)
			free(rooms[i]);
    return 0;
}

int get_rooms_server(int sock)	//Отправка списка комнат через сокет sock, аргумент args не используется
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

int get_name_server(int sock)  //Отправка наименования сервера
{
    char buf[MAXNICKLEN];
	strncpy(buf, nickname, MAXNICKLEN);
    send_message(sock, buf);
    return 0;
}

int send_message_server(int sock)	//Принятие сообщения сервером
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

int get_new_messages_server(int sock)	//Отправка недостаюших сообщений клиенту. Аргумент args не используется
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
	goto_message(room_fd[room], room_number[room] - count);
	struct s_message msg;
	//printf("%i|", room_number[room]-count);
	for (int i = 0; i < room_number[room]-count ; i++)
	{	
		//printf("%i ", i);
		//Считывание сообщения
		read_single_message(room_fd[room], &msg);
  	  	//Передача сообщений
		strncpy(buf, msg.datetime, strlen(msg.datetime)+1);
		send_message(sock, buf);
		strncpy(buf, msg.nickname, strlen(msg.nickname)+1);
		send_message(sock, buf);
		strncpy(buf, msg.msg_text, strlen(msg.msg_text)+1);
		send_message(sock, buf);
	}
	return 1;	
}


int ping_server(int sock)
{
	char buf[] = "pong";
	send_message(sock, buf);
}
