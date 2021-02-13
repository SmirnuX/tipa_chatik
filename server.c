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
				room_number[room_count] = count_messages(room_fd[room_count]);
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
		printf("%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i], lseek(room_fd[i], 0, SEEK_END), room_number[i]);
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
		printf(	DEFAULT BRIGHT 	
						"============================================================\n"	
				DEFAULT	"Доступны следующие функции:\n"
						"1. Создать новую комнату\n"
						"2. Написать сообщение в одну из комнат\n"
						"3. Удалить комнату\n"
				BRIGHT	"0. Закрыть сервер\n"
						"============================================================\n" DIM);
		erase_lines(SERVER_MENU_LINES);
		int n = 0;
		ioctl(STDIN_FILENO, FIONREAD, &n);	//Считываем количество бит, которые можно прочитать
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
					printf( DEFAULT
							WHITE BRIGHT "============================================================\n"
							DEFAULT "\tСоздание новой комнаты\n"
							DIM		"Введите название комнаты и нажмите ENTER.\n"
									"Для отмены создания комнаты сотрите все символы и нажмите ENTER\n"
							BRIGHT  "============================================================\n" DEFAULT);
					if (fgets(buf, MAXBUFFER, stdin) > 0)
					{
						erase_lines(SERVER_NEW_ROOM_LINES + 1);
						if (buf[0] != '\n')
						{
							remove_new_line(buf);
							if (room_count == MAXROOMS)
							{
								printf(RED "\n Достигнуто максимальное количество комнат.\n" WHITE);
							}
							else
							{
								int i;
								for (i = 0; i < room_count; i++)
								{
									if (strcmp(buf, rooms[i]) == 0)
										break;
								}
								if (i < room_count)	//Если найдено совпадение
								{
									printf(RED "\n Комната с таким названием уже существует.\n" WHITE);
								}
								else
								{
									room_fd[room_count+1] = open(buf, O_CREAT | O_RDWR, PERMISSION);
									if (rooms[room_count+1] == NULL)
										rooms[room_count+1] = malloc(sizeof(char) * MAXNICKLEN);
									strncpy(rooms[room_count+1], buf, MAXNICKLEN);
									if (room_fd[room_count+1] < 0)
									{
										perror(RED "\n Ошибка создания комнаты:" WHITE);
									}
									else
									{
										room_count++;
										server_time = time(NULL);	//Обновление "версии" сервера
										printf(	"\n Добавлена комната %s.\n"
												"Время обновления: %li\n", rooms[room_count], server_time);
										rooms_sort();
									}
 								}
							}
						}
					}
					break;
					case '2':	//===Отправка сообщения===
					printf( DEFAULT
							WHITE BRIGHT "\n============================================================\n"
							DEFAULT "\t\tСписок комнат сервера %s\n", nickname);            
					for (int i = 0; i < room_count; i++)
						printf("%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i], lseek(room_fd[i], 0, SEEK_END), room_number[i]);
					printf( BRIGHT  "\t0. Отменить написание сообщения\n"
							RED  	"На время написания сообщения работа сервера приостановлена\n"
							WHITE	"============================================================\n"
							DEFAULT "Введите номер выбранной комнаты: ");
					fgets(buf, MAXBUFFER, stdin);
					erase_lines(SERVER_CHOOSE_ROOM_LINES + 1 + room_count);
					choice = atoi(buf);
					if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
					{
						clear()
						printf(BRIGHT RED"Неправильно набран номер комнаты.\n\n"DEFAULT WHITE);
					}
					else if (choice != 0)
					{
						choice--;
						printf( DEFAULT
								WHITE BRIGHT"=======================================================================\n"
								DEFAULT     "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
								DIM         "\tДля отмены отправки сообщения сотрите все символы и нажмите ENTER.\n"
								WHITE BRIGHT"=======================================================================\n"
								DEFAULT);  
						if (fgets(buf, MAXBUFFER, stdin) > 0)
						{
							erase_lines(SERVER_WRITE_MESSAGE_LINES + 1);
							if (buf[0] != '\n')
							{
								char s_time[MAXBUFFER];
								time_t timer = time(NULL);
								strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
								buf[MAXBUFFER - 1] = '\0';
								write_message(room_fd[choice], s_time, nickname, buf, ++room_number[choice]);
								printf(	"\n Сообщение отправлено.\n");
							}
						}
					}
					break;
					case '3':	//===Удаление сообщения===
					printf( WHITE BRIGHT "\n============================================================\n"
							DEFAULT "\t\tСписок комнат сервера %s\n", nickname);            
					for (int i = 0; i < room_count; i++)
						printf("%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i], lseek(room_fd[i], 0, SEEK_END), room_number[i]);
					printf( BRIGHT  "\t0. Отменить удаление комнаты\n"
							RED  	"На время удаления комнаты работа сервера приостановлена\n"
							WHITE	"============================================================\n"
							DEFAULT "Введите номер удаляемой комнаты: ");
					fgets(buf, MAXBUFFER, stdin);
					erase_lines(SERVER_CHOOSE_ROOM_LINES + 1 + room_count);
					choice = atoi(buf);
					if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
					{
						clear()
						printf(BRIGHT RED"Неправильно набран номер комнаты.\n\n"DEFAULT WHITE);
					}
					else if (choice != 0)
					{
						choice--;
						close(room_fd[choice]);
						if (remove(rooms[choice]) == -1)
						{
							perror( RED "Ошибка удаления: " WHITE);
						}
						else
						{
							server_time = time(NULL);	//Обновление "версии" сервера
							printf(	"\n Удалена комната %s.\n"
									"Время обновления: %li\n", rooms[room_count], server_time);
							room_fd[choice] = -1;
							free(rooms[choice]);
							rooms[choice] = NULL;
							room_number[choice] = 0;
							rooms_sort();
							room_count--;
						}
					}
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
							get_message(fd, buf);
							//Идентификация команды
							for (int j = 0; j < CMD_COUNT; j++)	
							{
								if (strcmp(buf, server_cmd_strings[j]) == 0)	
								{
									printf("[%i]Выполняется %s \n",fd ,buf);
									snprintf(buf, MAXBUFFER, "%li", server_time);
									send_data(fd, buf);	//Отправляем "версию" сервера
									get_message(fd, buf);
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
	for (int i=0; i<MAXROOMS; i++)	
		if (rooms[i] != NULL)
			free(rooms[i]);
    return 0;
}

int get_rooms_server(int sock)	//Отправка списка комнат через сокет sock
{
	char buf[MAXBUFFER];
	snprintf(buf, MAXBUFFER, "%i", room_count);	//Отправка количества комнат
	send_data(sock, buf);
	for (int i=0; i<room_count; i++)	
	{
		strncpy(buf, rooms[i], MAXNICKLEN);
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
	get_message(sock, buf);	//Получение номера комнаты 
	int room = atoi(buf);
	//Время получения сообщения сервером
	time_t timer = time(NULL);
	strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", localtime(&timer));
	printf(DEFAULT BLUE"%s\n", s_time);
	//Никнейм отправителя
	get_message(sock, nickname);
	printf(DEFAULT BRIGHT" %s\n", nickname);
	//Сообщение
	get_message(sock, buf);
	printf(DEFAULT"%s\n", buf);
	write_message(room_fd[room], s_time, nickname, buf, ++room_number[room]);
}

int get_new_messages_server(int sock)	//Отправка недостаюших сообщений клиенту через сокет sock
{
    char buf[MAXBUFFER];
    //Получение комнаты
	int room = atoi(get_message(sock, buf));
    //Получение количества сообщений
    int count = atoi(get_message(sock, buf));	
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", room_number[room]);
    send_data(sock, buf);
	//Перемещение к первому передаваемому сообщению
	goto_message(room_fd[room], room_number[room] - count);
	struct s_message msg;
	for (int i = 0; i < room_number[room]-count ; i++)
	{	
		//Считывание сообщения
		read_single_message(room_fd[room], &msg);
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
		for (int j = i; j < MAXROOMS-1; j++)
		{
			if (rooms[j] == NULL && rooms[j+1] != NULL)
				rooms_swap(j, j+1);
			else if (rooms[j] != NULL && rooms[j+1] != NULL)
			{
				if (strcmp(rooms[j], rooms[j+1]) > 0)
					rooms_swap(j, j+1);
			}
		}
	}
}

void rooms_swap(int a, int b)
{
	int tmp_fd, tmp_n;
	char* tmp_str;
	//Для этого вообще то структуры придумали
	tmp_str = rooms[a];
	tmp_fd = room_fd[a];
	tmp_n = room_number[a];
	rooms[a] = rooms[b];
	room_fd[a] = room_fd[b];
	room_number[a] = room_number[b];
	rooms[b] = tmp_str;
	room_fd[b] = tmp_fd;
	room_number[b] = tmp_n;
}