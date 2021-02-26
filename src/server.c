// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int server(struct s_connection* connection)
{
	//signal(SIGPIPE, SIG_DFL);
    room_count = 0;
	for (int i=0; i<MAXROOMS; i++)
		rooms[i] = NULL;
    DIR* directory = opendir("server_history");
	if (directory==NULL)
	{
		mkdir(".files", FOLDERPERMISSION);
		directory = opendir("server_history");
		if (directory==NULL)
		{
			ui_show_error("Ошибка открытия директории \"server_history\".", 1);
			return 2;
		}
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
				if (rooms[room_count] == NULL)	//Инициализация комнаты
				{
					rooms[room_count] = malloc(sizeof(struct s_room));
					rooms[room_count]->name = malloc(sizeof(char) * MAXNICKLEN);
					for (int i=0; i < MAXROOMS; i++)
						rooms[room_count]->file_names[i] = NULL;
				}
				strncpy(rooms[room_count]->name,(*dir_ptr).d_name, MAXNICKLEN);
				refresh_files_room(room_count);
				rooms[room_count]->fd = open(rooms[room_count]->name, O_RDWR);
				rooms[room_count]->msg_count = count_messages(rooms[room_count]->fd);
				room_count++;
				if (room_count == MAXROOMS)
					break;
			}
		}
		dir_ptr=readdir(directory);	//Переходим к следующему файлу
	}
	printf( WHITE BRIGHT "\n============================================================\n"
				DEFAULT "\t\tСписок комнат сервера %s\n", nickname);      
	rooms_sort();	//Сортировка комнат по алфавиту      
	for (int i = 0; i < room_count; i++)
		printf("%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i]->name, lseek(rooms[i]->fd, 0, SEEK_END), rooms[i]->msg_count);
	printf(	WHITE	"============================================================\n");
	closedir(directory);
	//Подготовка списка соединений
	fd_set connections_set;	//Набор файловых дескрипторов соединений
	fd_set temp_connections_set;	//Временный набор дескрипторов	
	FD_ZERO(&connections_set);	//Обнуление набора
	FD_SET(connection->sock, &connections_set);
	int maxfd = connection->sock + 1;
	struct timeval timeout;
    listen(connection->sock, MAXQUEUE);
	server_time = time(NULL);
	printf(	DIM "Сервер запущен, время запуска: %li\n", server_time);
	int running = 1;
    while (running)
    {
		char buf[MAXBUFFER];
		int n = 0;
		ioctl(STDIN_FILENO, FIONREAD, &n);	//Считываем количество бит, которые можно прочитать
		server_ui_main_menu();
		if (n != 0)
		{
			
			int choice = getchar();
			prev_line()
			erase_line()
			if (choice != '\n')
			{
				while (getchar() !='\n');   //Очистка буфера
				switch (choice)
				{
					case '1':	////===Создание новой комнаты===	
					server_ui_create_room();
					break;
					case '2':	//===Отправка сообщения===
					server_ui_send_message();
					break;
					case '3':	//===Просмотр файлов===
					server_ui_view_files();
					break;
					case '4':	//===Удаление комнаты===
					server_ui_delete_room();
					break;
					case '0':
					printf("Закрытие сервера...\n");
					running = 0;
					break;
				}	
			}
		}
		if (!running)
			break;
		timeout.tv_usec = TIMEOUT_MS;
		timeout.tv_sec = TIMEOUT_S;
		//Копирование набора файловых дескрипторов
		FD_ZERO(&temp_connections_set);
		for (int fd = 0; fd < maxfd+1; fd++)
		{
			if (FD_ISSET(fd, &connections_set))
				FD_SET(fd, &temp_connections_set);
		}
		if (select(maxfd + 1, &temp_connections_set, NULL, NULL, &timeout) > 0)
		{
			//Опрос всех сокетов
			for (int fd = 0; fd < maxfd+1; fd++)
			{
				if (FD_ISSET(fd, &temp_connections_set))
				{
					if (fd == connection->sock)	//Активен сокет сервера
					{
						//Принимаем соединение
						errno = 0;
						int client_socket;
						struct sockaddr_in client_address;
						int len = sizeof(client_address);
						client_socket = accept(connection->sock, (struct sockaddr*) &client_address, &len);
						if (errno == EAGAIN)
						{
							errno = 0;
							continue;
						}
						else
						{
							//Добавление соединения в набор
							FD_SET(client_socket, &connections_set);
							if (client_socket > maxfd)
								maxfd = client_socket;
							printf("[%i]Добавлено новое соединение\n", client_socket);
						}
					}
					else	//Активен один из клиентов
					{				
						ioctl(fd, FIONREAD, &n);	//Считываем количество бит, которые можно прочитать
						if (n != 0)
						{
							//Считывание команды и ее выполнение
							get_data(fd, buf);
							//Идентификация команды
							for (int j = 0; j < CMD_COUNT; j++)	
							{
								if (strcmp(buf, server_cmd_strings[j]) == 0)	
								{
									printf("[%i]Выполняется %s \n",fd ,buf);
									snprintf(buf, MAXBUFFER, "%li", server_time);
									send_data(fd, buf);	//Отправляем "версию" сервера
									get_data(fd, buf);
									if (buf[0] != '0')	//Если "версия" сервера совпадает с клиентом - выполняем команду
										server_cmd_functions[j](fd);	
									break;
								}
							}
						}
						else
						{
							close(fd);
							FD_CLR(fd, &connections_set);
							printf("[%i]Закрыто соединение\n", fd);
						}
					}
				}
			}	
		}	
	}
	//Освобождение памяти
	for (int i = 0; i < MAXROOMS; i++)	
		if (rooms[i] != NULL)
		{
			if (rooms[i]->fd != -1)
                close(rooms[i]->fd);
			for (int j=0; j < MAXROOMS; j++)
				if (rooms[i]->file_names[j] != NULL)
					free(rooms[i]->file_names[j]);
			free(rooms[i]->name);
			free(rooms[i]);
		}
    return 0;
}

int get_rooms_server(int sock)	//Отправка списка комнат через сокет sock
{
	char buf[MAXBUFFER];
	snprintf(buf, MAXBUFFER, "%i", room_count);	//Отправка количества комнат
	send_data(sock, buf);
	for (int i=0; i<room_count; i++)	
	{
		strncpy(buf, rooms[i]->name, MAXNICKLEN);
		send_data(sock, buf);	//Отправка названий комнат
	}
}

int get_name_server(int sock)  //Отправка наименования сервера клиенту через сокет sock
{
    char buf[MAXNICKLEN];
	strncpy(buf, nickname, MAXNICKLEN);
	buf[MAXNICKLEN-1] = '\0';
    send_data(sock, buf);
    return 0;
}

int send_message_server(int sock)	//Получение сообщения сервером через сокет sock
{
	char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	get_data(sock, buf);	//Получение номера комнаты 
	int room = atoi(buf);
	//Время получения сообщения сервером
	time_t timer = time(NULL);
	strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
	printf(DEFAULT BLUE"%s\n", s_time);
	//Никнейм отправителя
	get_data(sock, nickname);
	printf(DEFAULT BRIGHT" %s\n", nickname);
	//Сообщение
	get_data(sock, buf);
	printf(DEFAULT"%s\n", buf);
	write_message(rooms[room]->fd, s_time, nickname, buf, ++(rooms[room]->msg_count));
}

int get_new_messages_server(int sock)	//Отправка недостаюших сообщений клиенту через сокет sock
{
    char buf[MAXBUFFER];
    //Получение комнаты
	int room = atoi(get_data(sock, buf));
    //Получение количества сообщений
    int count = atoi(get_data(sock, buf));	
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", rooms[room]->msg_count);
    send_data(sock, buf);
	//Перемещение к первому передаваемому сообщению
	goto_message(rooms[room]->fd, rooms[room]->msg_count - count);
	struct s_message msg;
	for (int i = 0; i < rooms[room]->msg_count-count ; i++)
	{	
		//Считывание сообщения
		read_single_message(rooms[room]->fd, &msg);
  	  	//Передача сообщений
		strncpy(buf, msg.datetime, strlen(msg.datetime)+1);
		send_data(sock, buf);
		strncpy(buf, msg.nickname, strlen(msg.nickname)+1);
		send_data(sock, buf);
		strncpy(buf, msg.msg_text, strlen(msg.msg_text)+1);
		send_data(sock, buf);
	}
	return 0;	
}

int send_data(int sock, char* str) //Отправка произвольного сообщения str БЕЗ попытки переподключения
{
    ssize_t a = write(sock, str, strlen(str)+1);  
    return (int) a;
}

void erase_lines(int n)	//Перемещение курсора на n строк наверх (и удаление этих строк)
{
	erase_line()
	for (int i = 0; i < n; i++)
	{
		prev_line()
		erase_line()
	}
}

void rooms_sort()	//Сортировка списка комнат
{
	for (int i = 0; i < MAXROOMS-1; i++)
	{
		for (int j = 0; j < MAXROOMS-1-i; j++)
		{
			if (rooms[j] == NULL && rooms[j+1] != NULL)
				rooms_swap(j, j+1);
			else if (rooms[j] != NULL && rooms[j+1] != NULL)
			{
				if (strcmp(rooms[j]->name, rooms[j+1]->name) > 0)
					rooms_swap(j, j+1);
			}
		}
	}
}

void rooms_swap(int a, int b)
{
	struct s_room* tmp;
	tmp = rooms[a];
	rooms[a] = rooms[b];
	rooms[b] = tmp;
}

int get_files_server(int sock)
{
	//Синтаксис: /getfiles <room> <pagenumber> (pagenumber - номер страницы из 10 файлов)
	char buf[MAXBUFFER];
    //Получение комнаты
	int room = atoi(get_data(sock, buf));
	refresh_files_room(room);
    //Получение номера страницы
    int page = atoi(get_data(sock, buf));
    //Отправка количества файлов всего
    snprintf(buf, MAXBUFFER, "%i", rooms[room]->file_count);
    send_data(sock, buf);
	//Передача названий файлов
	for (int i = (page-1)*10; i < page*10; i++)
	{	
		if (i == MAXFILES || i == rooms[room]->file_count)
			break;
		send_data(sock, rooms[room]->file_names[i]);
	}
	return 0;
}

int download_file_server(int sock)
{
	//Загрузка файла с сервера. Синтаксис: /downloadfile <room> <number>
	char buf[MAXBUFFER];
    //Получение комнаты
	int room = atoi(get_data(sock, buf));
    //Получение номера файла
    int number = atoi(get_data(sock, buf));
	if (number > rooms[room]->file_count)
	{
		send_data(sock, "-1");
		return 0;
	}
	chdir(".files");	//TODO - вставить проверку на существование и создание
	chdir(rooms[room]->name);
	struct stat stat_file;
	lstat(rooms[room]->file_names[number], &stat_file);	//Получаем размер файла
	long size = stat_file.st_size;
	snprintf(buf, MAXBUFFER, "%li", size);
	send_data(sock, buf);
	int file = open(rooms[room]->file_names[number], O_RDONLY);
	if (file < 0)
	{
		chdir("../..");
		send_data(sock, "1");
		refresh_files_room(room);
		return -1;
	}
	send_data(sock, "0");
	long sent_data = 0;
	printf(	WHITE	"Отправка файла %s\n"
			GREEN	"[                    ]    %%", rooms[room]->file_names[number]);
	int read_count = 0;
	do
	{
		erase_line()
		read_count = read(file, buf, MAXBUFFER-1);
		char packet_size_str[MAXBUFFER];
        snprintf(packet_size_str, MAXBUFFER, "%i", read_count);
        send_data(sock, packet_size_str);
        write(sock, buf, read_count);
		sent_data += read_count;
		printf("[");
		int percent = sent_data * 100 / size;
		for (int i=0; i < 20; i++)
		{
			if (i < percent / 5)
				printf("#");
			else
				printf(" ");
		}
		printf("] %3i%%", percent);
		get_data(sock, buf);
	}
	while (read_count == MAXBUFFER-1); 	//TODO - вставить проверку на ошибки
	erase_line()
	printf("Файл отправлен.\n");
	close(file);
	chdir("../..");
}

int send_file_server(int sock)	//ВЫДЕЛЕНИЕ ПАМЯТИ
{
	//Загрузка файла на сервер. Синтаксис: /sendfile <room> <nickname> <filename> <filesize>
	char buf[MAXBUFFER];
	char filename[MAXBUFFER];
	char nick[MAXNICKLEN];
    //Получение комнаты
	int room = atoi(get_data(sock, buf));
	//Получение никнейма отправителя
	get_data(sock, buf);
	strncpy(nick, buf, MAXNICKLEN);
    //Получение названия файла
    get_data(sock, buf);
	strncpy(filename, buf, MAXNICKLEN);
	long size = atol(get_data(sock, buf));
	if (chdir(".files") == -1)
	{
		mkdir(".files", FOLDERPERMISSION);
		chdir(".files");
	}
	if (chdir(rooms[room]->name) == -1)
	{
		mkdir(rooms[room]->name, FOLDERPERMISSION);
		chdir(rooms[room]->name);
	}
	int error = 0;	//Код ошибки: 0 - ошибки нет, 1 - файл с таким названием уже есть, 2 - не хватает места в массиве
	if (rooms[room]->file_count == MAXROOMS)
		error = 2;
	else
	{
		for (int i = 0; i < rooms[room]->file_count; i++)	
		{
			if (strcmp(rooms[room]->file_names[i], filename) == 0)
			{
				int t_file = open(rooms[room]->file_names[i], O_RDONLY);
				if (t_file > 0)
				{
					close(t_file);
					error = 1;
				}
				break;
			}
		}
	}
	snprintf(buf, MAXBUFFER, "%i", error);	//Отправка номера ошибки
	send_data(sock, buf);
	if (error != 0)
	{
		chdir("../..");
		return 1;
	}
	int file = open(filename, O_CREAT | O_WRONLY, PERMISSION);
	long received_data = 0;
	printf(	WHITE DEFAULT	"Получение файла %s\n"
			GREEN			"[                    ]    %%", filename);
	rooms[room]->file_names[ (rooms[room]->file_count) ] = malloc(sizeof(char)*MAXNICKLEN);
	strncpy( rooms[room]->file_names[rooms[room]->file_count++], filename, MAXNICKLEN);
	for (received_data = 0; received_data < size; )
	{
		erase_line()
		get_data(sock, buf);
		int packet_size = atoi(buf);
		get_ndata(sock, buf, packet_size);
		received_data += write(file, buf, packet_size);
		printf("[");
		int percent = received_data * 100 / size;
		for (int i=0; i < 20; i++)
		{
			if (i < percent / 5)
				printf("#");
			else
				printf(" ");
		}
		printf("] %3i%%", percent);
		send_data(sock, "1");
		//TODO - проверка на удачную запись
	}
	erase_line()
	printf("Файл получен.\n");
	close(file);
	chdir("../..");
	char s_time[MAXBUFFER];
	time_t timer = time(NULL);
	strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
	snprintf(buf, MAXBUFFER, GREEN"Отправлен " BRIGHT "%s\n" WHITE DEFAULT, filename);
	write_message(rooms[room]->fd, s_time, nick, buf, ++(rooms[room]->msg_count));
}
    
void refresh_files_room(int room)	//Обновить список файлов, хранящихся в комнате
{
	if (chdir(".files") != 0)
	{
		if (errno == ENOENT)
		{
			mkdir(".files", FOLDERPERMISSION);
			chdir(".files");
			errno = 0;
		}
	}
	DIR* internal_directory = opendir(rooms[room]->name);
	if (chdir(rooms[room]->name) != 0)
	{
		if (errno == ENOENT)
		{
			mkdir(rooms[room]->name, FOLDERPERMISSION);
			chdir(rooms[room]->name);
			errno = 0;
		}
	}
	rooms[room]->file_count = 0;
	if (internal_directory != NULL)
	{
		struct dirent* internal_dir_ptr;
		internal_dir_ptr = readdir(internal_directory);
		while (internal_dir_ptr!=NULL)	
		{
			if ((*internal_dir_ptr).d_name[0] != '.')	//Пропуск скрытых файлов
			{
				if (rooms[room]->file_count < MAXFILES - 1)
				{
					int max_dir_len = sizeof((*internal_dir_ptr).d_name) - 1;
					(*internal_dir_ptr).d_name[max_dir_len] = '\0';
					if (strlen((*internal_dir_ptr).d_name) < MAXNICKLEN)
					{
						printf("%s", (*internal_dir_ptr).d_name);
						if (rooms[room]->file_names[rooms[room]->file_count] == NULL)
							rooms[room]->file_names[rooms[room]->file_count] = malloc(sizeof(char)*MAXNICKLEN);	//Выделение памяти
						strncpy(rooms[room]->file_names[rooms[room]->file_count], (*internal_dir_ptr).d_name, MAXNICKLEN);
						rooms[room]->file_count++;
					}
				}
				else
					break;
			}
			internal_dir_ptr = readdir(internal_directory);
		}
	}
	closedir(internal_directory);
	for (int i = rooms[room]->file_count; i < MAXROOMS; i++)
	{
		free(rooms[room]->file_names[i]);
		rooms[room]->file_names[i] = NULL;
	}
	chdir("../..");
}