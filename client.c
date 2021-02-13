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
    {
        rooms[i] = NULL;
        room_number[i] = 0;
        room_fd[i] = -1;
    }
    room_count = 0;
    while(1)
    {
        if (selected_room == -1)
        {
            //===Меню выбора комнаты===   
            //Получение названия сервера
            if (server_name != NULL)   
                free(server_name);
            server_name = get_name_client(connection);
            //Получение списка комнат
            printf( WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", server_name);            
            get_rooms_client(connection);
            for (int i = 0; i < MAXROOMS; i++)
            {
                if (room_fd[i] > 0)
                {
                    close(room_fd[i]);
                    room_fd[i] = -1;
                }
            }
            for (int i = 0; i < room_count; i++)
            {
                room_fd[i] = open(rooms[i], O_CREAT | O_RDWR, PERMISSION);  //Создание файлов истории
                lseek(room_fd[i], 0, SEEK_SET);
                room_number[i] = count_messages(room_fd[i]);
                printf("\t%i. %s (%i сохр. сообщ-й)\n", i+1, rooms[i], room_number[i]);
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
            printf("\tСообщения %s\n\n", rooms[selected_room]);
            //Отображение истории сообщений
            if (get_new_messages_client(connection, selected_room, room_number[selected_room]) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                selected_room = -1;
                continue;
            }
            lseek(room_fd[selected_room], 0, SEEK_SET);
            clear()
            read_messages(room_fd[selected_room]);
            //===Меню выбора действия===
            printf( WHITE BRIGHT "============================================================\n"
            DEFAULT "\tДействия в \"%s\"\n"
                    "\t1. Написать сообщение\n"
                    "\t2. Обновить\n"
            BRIGHT  "\t0. Выйти из комнаты\n"
                    "============================================================\n"
            DEFAULT "Введите номер выбранного действия: ", rooms[selected_room]);
            char choice;
            
            choice = getchar();
            while (getchar() !='\n');   //Очистка буфера
            int error;
            switch (choice)
            {
                case '0':   //Возвращение к списку комнат
                selected_room = -1;
                clear()
                break;
                case '1':   //Переход к написанию сообщений
                clear()
                if (get_new_messages_client(connection, selected_room, room_number[selected_room]) == ECONNREFUSED)
                {
                    clear()
                    printf ("Произошли изменения на сервере. Переподключение...\n\n");
                    selected_room = -1;
                    break;
                }
                lseek(room_fd[selected_room], 0, SEEK_SET);
                read_messages(room_fd[selected_room]);
                printf( 
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT     "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
                DIM         "\tДля отмены отправки сообщения сотрите все символы и нажмите ENTER.\n"
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT);
                char buf[MAXBUFFER];    
                if (fgets(buf, MAXBUFFER, stdin) > 0)
                    if (buf[0] != '\n')
                        if (send_message_client(connection, selected_room, nickname, buf) == ECONNREFUSED)
                        {
                            clear()
                            printf ("Произошли изменения на сервере. Переподключение...\n\n");
                            selected_room = -1;
                        }
                break;
                case '2':
                clear()
                continue;
                break; 
            }
        }
    }
    //Освобождение памяти
    for (int i = 0; i < MAXROOMS; i++)
    {
        if (rooms[i] != NULL)
            free(rooms[i]);
        if (room_fd[i] != -1)
            close(room_fd[i]);
    }

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
        if (rooms[i] == NULL)   //Если память еще не выделена
            rooms[i] = malloc(sizeof(char) * MAXNICKLEN);  
        strncpy(rooms[i], buf, MAXNICKLEN);
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
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data_safe(connection, buf);
    //Отправка никнейма
    send_data_safe(connection, nickname);
    //Отправка сообщения
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
    	write_message(room_fd[room], s_time, nickname, buf, ++room_number[room]);   
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
    for (i=0; i<MAXBUFFER-1; i++)
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