// SPDX-License-Identifier: CPOL-1.02
#include "main.h"
int client(struct s_connection* connection)
{
    char* server_name = NULL;
    int selected_room = -1; //Выбранная комната
    signal(SIGPIPE, SIG_IGN);	//Игнорируем ошибки при записи в сокет

    if (chdir("client_history") == -1)  //Переход в папку, хранящую историю сообщений
    {
        int error = 0;
        if (errno == ENOENT)    //Если папка не существует
        {
            errno = 0;
            mkdir("client_history", FOLDERPERMISSION);
            chdir("client_history");
        }
        if (errno != 0) //Для всех остальных ошибок, либо при ошибке при создании папки
        {
            perror(RED "Ошибка открытия папки \"client_history\"");
            return 0;
        }
    }
    for (int i = 0; i < MAXROOMS; i++)
        rooms[i] = NULL;
    room_count = 0;
    while(1)
    {
        if (selected_room == -1)
        {
            //===Меню выбора комнаты===   
            //Получение названия сервера
            if (server_name != NULL)
            {   
                free(server_name);
                server_name = NULL;
            }
            server_name = get_name_client(connection);
            //Получение списка комнат
            printf( WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", server_name);            
            get_rooms_client(connection);
            for (int i = 0; i < MAXROOMS; i++)
            {
                if (rooms[i] == NULL)
                    continue;
                if (rooms[i]->fd > 0)
                {
                    close(rooms[i]->fd);
                    rooms[i]->fd = -1;
                }
            }
            for (int i = 0; i < room_count; i++)
            {
                rooms[i]->fd = open(rooms[i]->name, O_CREAT | O_RDWR, PERMISSION);  //Создание файлов истории
                lseek(rooms[i]->fd, 0, SEEK_SET);
                rooms[i]->msg_count = count_messages(rooms[i]->fd);
                printf("\t%i. %s (%i сохр. сообщ-й)\n", i+1, rooms[i]->name, rooms[i]->msg_count);
            }
            printf( BRIGHT  "\t0. Закрыть программу\n"
                            "============================================================\n"
                    DEFAULT "Введите номер выбранной комнаты: ");
            char buf[MAXBUFFER];
            fgets(buf, MAXBUFFER, stdin);
            int choice = atoi(buf);
            if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
            {
                clear()
                printf(BRIGHT RED"Неправильно набран номер комнаты.\n\n"DEFAULT WHITE);
                continue;
            }
            if (choice == 0)
                break;
            else
                selected_room = choice-1;
        }
        else
        {
            printf("\tСообщения %s\n\n", rooms[selected_room]->name);
            //Отображение истории сообщений
            if (get_new_messages_client(connection, selected_room, rooms[selected_room]->msg_count) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                selected_room = -1;
                continue;
            }
            lseek(rooms[selected_room]->fd, 0, SEEK_SET);
            clear()
            read_messages(rooms[selected_room]->fd);
            //===Меню выбора действия===
            printf( WHITE BRIGHT "============================================================\n"
            DEFAULT "\tДействия в \"%s\"\n"
                    "\t1. Написать сообщение\n"
                    "\t2. Обновить\n"
                    "\t3. Загрузить файл на сервер\n"
                    "\t4. Скачать файл с сервера\n"
            BRIGHT  "\t0. Выйти из комнаты\n"
                    "============================================================\n"
            DEFAULT "Введите номер выбранного действия: ", rooms[selected_room]->name);
            char choice = getchar();
            while (getchar() !='\n');   //Очистка буфера
            int error;
            char buf[MAXBUFFER];    
            switch (choice)
            {
                case '0':   //Возвращение к списку комнат
                selected_room = -1;
                clear()
                break;
                case '1':   //Переход к написанию сообщений
                clear()
                if (get_new_messages_client(connection, selected_room, rooms[selected_room]->msg_count) == ECONNREFUSED)
                {
                    clear()
                    printf ("Произошли изменения на сервере. Переподключение...\n\n");
                    selected_room = -1;
                    break;
                }
                lseek(rooms[selected_room]->fd, 0, SEEK_SET);
                read_messages(rooms[selected_room]->fd);
                printf( 
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT     "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
                DIM         "\tДля отмены отправки сообщения сотрите все символы и нажмите ENTER.\n"
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT);
                if (fgets(buf, MAXBUFFER, stdin) > 0)
                    if (buf[0] != '\n')
                    {
                        __fpurge(stdin);    //Очистка буфера
                        if (send_message_client(connection, selected_room, nickname, buf) == ECONNREFUSED)
                        {
                            clear()
                            printf ("Произошли изменения на сервере. Переподключение...\n\n");
                            selected_room = -1;
                        }
                    }
                break;
                case '2':   //Обновление сообщений
                clear()
                continue;
                case '3':   //Загрузка файла на сервер
                clear()
                chdir("..");
                getcwd(buf, MAXBUFFER);
                printf( 
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT     "\tВведите путь к загружаемому файлу и ENTER для отправки файла\n"
                DIM         "\tДля отмены отправки файла сотрите все символы и нажмите ENTER.\n"
                            "\tФайлы должны находиться в директории с чатом: %s\n"
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT, buf);
                if (fgets(buf, MAXBUFFER, stdin) > 0)
                    if (buf[0] != '\n')
                    {
                        __fpurge(stdin);    //Очистка буфера
                        remove_new_line(buf);
                        int file = open(buf, O_RDONLY);
                        if (file < 0)
                        {
                            clear()
                            perror("Ошибка открытия файла: ");
                            errno = 0;
                            pause();
                        }
                        else if (send_file_client(connection, selected_room, file, buf) == ECONNREFUSED)
                        {
                            clear()
                            printf ("Произошли изменения на сервере. Переподключение...\n\n");
                            selected_room = -1;
                        }
                    }
                chdir("client_history");
                break;
                case '4':   //Загрузка файлов с сервера
                client_ui_download_files(connection, &selected_room);
                break; 
            }
        }
    }
    //Освобождение памяти
    for (int i=0; i<MAXROOMS; i++)	
		if (rooms[i] != NULL)
		{
            if (rooms[i]->fd != -1)
                close(rooms[i]->fd);
			free(rooms[i]->name);
			free(rooms[i]);
		}
    if (server_name != NULL)   
        free(server_name);
    return 0;
}

int get_rooms_client(struct s_connection* connection) //Получение списка комнат от сервера через соединение connection. В процессе выделяется память, которая должна быть очищена! (rooms[i])
{
    char buf[MAXBUFFER];
    strncpy(buf, "/getrooms", MAXBUFFER);
    send_data_safe(connection, buf);
    server_time = atoi(get_message(connection->sock, buf)); //Получение "версии" сервера
    send_data_safe(connection, "1");
    int count = atoi(get_message(connection->sock, buf));
    for (int i=0; i<count; i++)
    {
        get_message(connection->sock, buf);      
        if (rooms[i] == NULL)	//Инициализация комнаты
        {
            rooms[i] = malloc(sizeof(struct s_room));
            rooms[i]->name = malloc(sizeof(char) * MAXNICKLEN);
            rooms[i]->fd = -1;
        }
        strncpy(rooms[i]->name, buf, MAXNICKLEN);
    }
    room_count = count;
    return 0;
}

int send_message_client(struct s_connection* connection, int room, char* nickname, char* message)    //Отправка сообщения message на сервер через соединение connection в комнату под номером room пользователем nickname
{
    char buf[MAXBUFFER];
    strncpy(buf, "/sendmessage", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_message(connection->sock, buf)) != server_time) //Проверка "версии" сервера
    {
        send_data_safe(connection, "0");   //Информируем сервер о старой версии
        return ECONNREFUSED;    //Отказано в соединении
    }
    send_data_safe(connection, "1");
    //Отправка номера комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка никнейма
    nickname[MAXNICKLEN-1] = '\0';
    send_data_safe(connection, nickname);
    //Отправка сообщения
    message[MAXBUFFER-1] = '\0';
    send_data_safe(connection, message);
    return 0;
}

int get_new_messages_client(struct s_connection* connection, int room, int count)  //Получение недостающих сообщений через соединение connection для комнаты под номером room, в которой на данный момент сохранено count сообщений
{
    char buf[MAXBUFFER];   
    strncpy(buf, "/getnewmessages", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_message(connection->sock, buf)) != server_time) //Проверка "версии" сервера
    {
        send_data_safe(connection, "0");   //Информируем сервер о старой версии
        return ECONNREFUSED;    //Отказано в соединении
    }
    send_data_safe(connection, "1");
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", count);
    send_data_safe(connection, buf);
    //Прием количества сообщений
    int target_count = atoi(get_message(connection->sock, buf));
    //Прием сообщений
    for(int i = count; i<target_count; i++)
    {
        char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	    //Получение сообщения
	    get_message(connection->sock, s_time);
	    get_message(connection->sock, nickname);
    	get_message(connection->sock, buf);
        //Запись сообщения в файл
    	write_message(rooms[room]->fd, s_time, nickname, buf, ++(rooms[room]->msg_count));   
    }
    return 0;
}

char* get_name_client(struct s_connection* connection)  //Получение наименования сервера через соединение connection. В процессе выделяется память, которая должна быть очищена! (возвращаемое значение)
{
    char buf[MAXBUFFER];
    strncpy(buf, "/getname", MAXBUFFER);
    send_data_safe(connection, buf);
    get_message(connection->sock, buf); //Получение "версии" сервера - в этой команде не используется
    send_data_safe(connection, "1");
    get_message(connection->sock, buf);
    char* res = malloc(sizeof(char)*MAXNICKLEN);
    strncpy(res, buf, MAXNICKLEN);
    return res;
}

int send_data_safe(struct s_connection* connection, char* str) //Отправка произвольного сообщения str с попыткой переподключения в случае неудачи
{
    //Проверка соединения
    fd_set connection_set;
	struct timeval timeout;
    //Попытка переподключиться
    while (send_data(connection->sock, str) <= 0)
    {
        printf("Соединению кранты\n");
        close(connection->sock);
        errno = 0;
        connection->sock = socket(AF_INET, SOCK_STREAM, 0);
        connect(connection->sock, connection->address, sizeof(*(connection->address)));
        perror("Переподключение: ");
    }
    return 0;
}

char* get_message(int socket, char* str)    //Прием произвольного сообщения
{
    int i;
    char temp;
    for (i=0; i<MAXBUFFER; i++)
    {  
        int j = read(socket, str+i, 1); 
        if (j <= 0) 
        {  
            str[i] = '\0';
            break;
        }
        if (str[i] == '\0')
            break;
    }
    return str;
}

int get_files_client(struct s_connection* connection, int room, int page)  //Получение списка файлов комнаты
{
    char buf[MAXBUFFER];   
    strncpy(buf, "/getfiles", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_message(connection->sock, buf)) != server_time) //Проверка "версии" сервера
    {
        send_data_safe(connection, "0");   //Информируем сервер о старой версии
        return ECONNREFUSED;    //Отказано в соединении
    }
    send_data_safe(connection, "1");
    //Отправка номера комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка номера страницы
    snprintf(buf, MAXBUFFER, "%i", page);
    send_data_safe(connection, buf);
    //Получение количества файлов
    int count = atoi(get_message(connection->sock, buf));
    rooms[room]->file_count = count;
    //Получение списка файлов
	for (int i = (page-1)*10; i < page*10; i++)
	{	
		if (i == count)
			break;
        get_message(connection->sock, buf);
        rooms[room]->file_names[i] = malloc(sizeof(char)*MAXNICKLEN);
	    strncpy(rooms[room]->file_names[i], buf, MAXNICKLEN);
	}
	return 0;
}

int download_file_client(struct s_connection* connection, int room, int number)
{
	//Загрузка файла с сервера. Синтаксис: /downloadfile <room> <number>
    char buf[MAXBUFFER];   
    strncpy(buf, "/downloadfile", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_message(connection->sock, buf)) != server_time) //Проверка "версии" сервера
    {
        send_data_safe(connection, "0");   //Информируем сервер о старой версии
        return ECONNREFUSED;    //Отказано в соединении
    }
    send_data_safe(connection, "1");
    //Отправка номера комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка номера страницы
    snprintf(buf, MAXBUFFER, "%i", number);
    send_data_safe(connection, buf);
	chdir("../downloads");
	//Получение размера файла
    long size = atol(get_message(connection->sock, buf));
	int file = open(rooms[room]->file_names[number], O_CREAT | O_WRONLY, PERMISSION);
	int received_data = 0;
	printf(	WHITE	"Получение файла %s\n"
			GREEN	"[                    ]    %%", rooms[room]->file_names[number]);
	for (int j = 0; j < size; j += MAXBUFFER-1)
	{
		erase_line()
		get_message(connection->sock, buf);
		received_data += write(file, buf, strlen(buf));
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
	}
	close(file);
	chdir("../client_history");
}

int send_file_client(struct s_connection* connection, int room, int file, char* filename)	
{
	//Загрузка файла на сервер. Синтаксис: /sendfile <room> <nickname> <filename> <filesize>
	char buf[MAXBUFFER]; 
    strncpy(buf, "/sendfile", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_message(connection->sock, buf)) != server_time) //Проверка "версии" сервера
    {
        send_data_safe(connection, "0");   //Информируем сервер о старой версии
        return ECONNREFUSED;    //Отказано в соединении
    }
    send_data_safe(connection, "1");
    //Отправка номера комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка никнейма
    strncpy(buf, nickname, MAXBUFFER);
    send_data_safe(connection, buf);
	//Отправка названия файла
    strncpy(buf, filename, MAXBUFFER);
    send_data_safe(connection, buf);
    //Отправка размера файла
    struct stat stat_file;
	fstat(file, &stat_file);	//Получаем размер файла
	long size = stat_file.st_size;
    snprintf(buf, MAXBUFFER, "%li", size);
    send_data_safe(connection, buf);
    int sent_data = 0;
	printf(	WHITE	"Отправка файла %s\n"
			GREEN	"[                    ]    %%", filename);
	int read_count = 0;
	do
	{
		erase_line()
		read_count = read(file, buf, MAXBUFFER-1);
		buf[MAXBUFFER-1] = '\0';
		send_data_safe(connection, buf);
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
	}
	while (read_count == MAXBUFFER-1); 	//TODO - вставить проверку на ошибки
    printf("\n");
	close(file);	
}

int client_ui_download_files(struct s_connection* connection, int* selected_room)
{
    int page = 1;
    //int running = 1;
    while (1)
    {
        //Получение списка файлов с сервера
        if (get_files_client(connection, *selected_room, page) == ECONNREFUSED)
        {
            clear()
            printf ("Произошли изменения на сервере. Переподключение...\n\n");
            *selected_room = -1;
            return -1;
        }
        printf( WHITE BRIGHT "\n============================================================\n"
                DEFAULT "\t\tСписок файлов %s\n", rooms[*selected_room]->name);            
        //Вывод списка
        for (int i = (page-1)*FILESPERPAGE; i < page*FILESPERPAGE; i++)
        {	
            if (i == MAXFILES || i == rooms[*selected_room]->file_count)
                break;
            printf("%i. %s\n", i+1, rooms[*selected_room]->file_names[i]);
        }

        printf( BRIGHT  "\t0. Отменить выбор файла\n");
        if (page == 1)
            printf(DIM);
        printf("\t<A"DEFAULT"\t Страница %i из %i \t", page, rooms[*selected_room]->file_count/FILESPERPAGE + 1);              
        if (page == rooms[*selected_room]->file_count/FILESPERPAGE + 1)
            printf(DIM);
        printf("D>\n"   BRIGHT "============================================================\n"
                DEFAULT "Введите номер выбранного файла или A/D для перехода на другую страницу: ");
        char buf[MAXBUFFER];
        fgets(buf, MAXBUFFER, stdin);
        int choice = atoi(buf);
        switch(buf[0])
        {
            case 'A':
            if (page != 0)
                page--;
            break;
            case 'D':
            if (page  < rooms[*selected_room]->file_count/FILESPERPAGE)
                page++;
            break;
            case '0':
            return 0;
            break;
            default:
            if (choice <= 0 || choice > rooms[*selected_room]->file_count)
            {
                clear()
                printf(BRIGHT RED"Неправильно набран номер файла.\n\n"DEFAULT WHITE);
                continue;
            }
            else if (download_file_client(connection, *selected_room, choice-1) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                *selected_room = -1;
                return -1;
            }       
        }
  
    }

}