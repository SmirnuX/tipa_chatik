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
            ui_show_error("Ошибка открытия папки \"client_history\".", 1);
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
            get_rooms_client(connection);   //Получение списка комнат
            for (int i = 0; i < MAXROOMS; i++)  //Закрытие ранее открытых файлов
            {
                if (rooms[i] == NULL)
                    continue;
                if (rooms[i]->fd > 0)
                {
                    close(rooms[i]->fd);
                    rooms[i]->fd = -1;
                }
            }
            int choice = client_ui_select_room(server_name);    //Вывод списка комнат
            if (choice < 0)
                continue;
            if (choice == 0)
                break;
            else
                selected_room = choice-1;
        }
        else
        {
            //Получение истории сообщений
            if (get_new_messages_client(connection, selected_room, rooms[selected_room]->msg_count) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                selected_room = -1;
                continue;
            }
            int choice = client_ui_select_action(selected_room);    //Вывод истории сообщений и меню выбора действий
            int error;
            char buf[MAXBUFFER];    
            switch (choice)
            {
                case 0:   //Возвращение к списку комнат
                selected_room = -1;
                clear()
                break;
                case 1:   //Переход к написанию сообщений
                clear()
                if (client_ui_send_message(selected_room, connection) < 0)
                {
                    printf ("Произошли изменения на сервере. Переподключение...\n\n");
                    selected_room = -1;
                }
                break;
                case 2:   //Обновление сообщений
                clear()
                continue;
                case 3:   //Загрузка файла на сервер
                clear()
                if (client_ui_send_file(selected_room, connection) < 0)
                    selected_room = -1;
                break;
                case 4:   //Загрузка файлов с сервера
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
            for (int j=0; j < MAXROOMS; j++)
				if (rooms[i]->file_names[j] == NULL)
					free(rooms[i]->file_names[j]);
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
    server_time = atoi(get_data(connection->sock, buf)); //Получение "версии" сервера
    send_data_safe(connection, "1");
    int count = atoi(get_data(connection->sock, buf));
    for (int i=0; i<count; i++)
    {
        get_data(connection->sock, buf);      
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
    if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
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
    if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
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
    int target_count = atoi(get_data(connection->sock, buf));
    //Прием сообщений
    for(int i = count; i<target_count; i++)
    {
        char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	    //Получение сообщения
	    get_data(connection->sock, s_time);
	    get_data(connection->sock, nickname);
    	get_data(connection->sock, buf);
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
    get_data(connection->sock, buf); //Получение "версии" сервера - в этой команде не используется
    send_data_safe(connection, "1");
    get_data(connection->sock, buf);
    char* res = malloc(sizeof(char)*MAXNICKLEN);
    strncpy(res, buf, MAXNICKLEN);
    return res;
}

int send_data_safe(struct s_connection* connection, char* str) //Отправка произвольного сообщения str с попыткой переподключения в случае неудачи
{
    int try = 0;
    while (send_data(connection->sock, str) <= 0)
    {
        client_ui_reconnect(0);
        close(connection->sock);
        errno = 0;
        connection->sock = socket(AF_INET, SOCK_STREAM, 0);
        int err = connect(connection->sock, connection->address, sizeof(*(connection->address)));
        if (err != 0)
        {
            try++;
            if (client_ui_reconnect(try) > 0)
                return 1; 
        }
    }
    return 0;
}

int send_ndata_safe(struct s_connection* connection, char* str, int n)  //Отправка сообщения str длиной n с попыткой переподключения в случае неудачи
{
    int try = 0;
    while (write(connection->sock, str, n) <= 0)
    {
        close(connection->sock);
        errno = 0;
        connection->sock = socket(AF_INET, SOCK_STREAM, 0);
        int err = connect(connection->sock, connection->address, sizeof(*(connection->address)));
        if (err != 0)
        {
            try++;
            if (client_ui_reconnect(try) > 0)
                return 1; 
        }
    }
    return 0;
}

char* get_data(int socket, char* str)    //Прием произвольного сообщения
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

char* get_ndata(int socket, char* str, int n)    //Прием сообщения размером n. В случае неудачи записывает в конце принятой части сообщения \0
{
    int i;
    char temp;
    for (i=0; i<n; i++)
    {  
        int j = read(socket, str+i, 1); 
        if (j <= 0) 
        {  
            str[i] = '\0';
            break;
        }
    }
    return str;
}



int get_files_client(struct s_connection* connection, int room, int page)  //Получение списка файлов комнаты
{
    char buf[MAXBUFFER];   
    strncpy(buf, "/getfiles", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
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
    int count = atoi(get_data(connection->sock, buf));
    rooms[room]->file_count = count;
    //Получение списка файлов
	for (int i = (page-1)*10; i < page*10; i++)
	{	
		if (i == count)
			break;
        get_data(connection->sock, buf);
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
    if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
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
    long size = atol(get_data(connection->sock, buf));
    //Получаем код ошибки
    get_data(connection->sock, buf);
    if (buf[0] != '0')
    {
        chdir("../client_history");
        ui_show_error("Файл удален с сервера.", 0);
        return 1;
    }
	int file = open(rooms[room]->file_names[number], O_CREAT | O_WRONLY, PERMISSION);
	long received_data = 0;
	printf(	WHITE	"Получение файла %s\n"
			GREEN	"[                    ]    %%", rooms[room]->file_names[number]);
	for (received_data = 0; received_data < size; )
	{
		erase_line()
		get_data(connection->sock, buf);
		int packet_size = atoi(buf);
		get_ndata(connection->sock, buf, packet_size);
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
        send_data(connection->sock, "1");
	}
    erase_line()
    printf("Файл загружен.\n");
	close(file);
	chdir("../client_history");
}

int send_file_client(struct s_connection* connection, int room, int file, char* filename)	
{
	//Загрузка файла на сервер. Синтаксис: /sendfile <room> <nickname> <filename> <filesize>
	char buf[MAXBUFFER]; 
    strncpy(buf, "/sendfile", MAXBUFFER);
    send_data_safe(connection, buf);
    if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
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
    //Получаем код ошибки
    get_data(connection->sock, buf);
    if (buf[0] != '0')
    {
        switch (buf[0])
        {
            case '1':
            ui_show_error("Файл с таким названием уже существует в этой комнате.", 0);
            break;
            case '2':
            ui_show_error("Достигнуто максимальное количество файлов в этой комнате.", 0);
            break;          
        }
        close(file);
        return 1;
    }
    long sent_data = 0;
	printf(	WHITE	"Отправка файла %s\n"
			GREEN	"[                    ]    %%", filename);
	int read_count = 0;
	do
	{
		erase_line()
		read_count = read(file, buf, MAXBUFFER);
        char packet_size_str[MAXBUFFER];
        snprintf(packet_size_str, MAXBUFFER, "%i", read_count);
        send_data_safe(connection, packet_size_str);
        send_ndata_safe(connection, buf, read_count);
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
		get_data(connection->sock, buf);
	}
	while (read_count == MAXBUFFER); 	//TODO - вставить проверку на ошибки
    printf("\n");
	close(file);
}