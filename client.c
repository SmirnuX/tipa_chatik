// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int client(struct s_connection* connection)
{
    int selected_room = -1; //Выбранная комната
    chdir("client_history");    //TODO - проверка на существование
    for (int i = 0; i < MAXROOMS; i++)
    {
        rooms[i] = malloc(sizeof(char) * MAXNICKLEN);
        room_number[i] = 0;
        room_fd[i] = -1;
    }
    room_count = 0;
    while(1)
    {
        if (selected_room == -1)
        {
            //===Меню выбора комнаты===
            char* server_name;
            //Получение названия сервера
            server_name = get_name_client(connection);
            //Получение списка комнат
            printf(DEFAULT "\t\tСписок комнат сервера %s\n", server_name);
            get_rooms_client(connection);
            for (int i = 0; i < room_count; i++)
            {
                if (room_fd[i] < 0) //Если файл для i-ой комнаты еще не создан
                    room_fd[i] = open(rooms[i], O_CREAT | O_RDWR, PERMISSION);  //Создание файлов истории
                //TODO - вставить проверку версий сервера и удаление/создание новых файлов
                printf("\t%i. %s\n", i+1, rooms[i]);
            }
            printf( BRIGHT "\t0. Закрыть програму.\n"
                    DEFAULT "Введите номер выбранной комнаты: ");
            char buf[MAXBUFFER];
            fgets(buf, MAXBUFFER, stdin);
            int choice = atoi(buf);
            if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
            {
                clear()
                printf(BRIGHT RED"Неправильно набран номер комнаты.\n\n");
                continue;   //Вторая попытка
            }
            printf("%i", choice);
            if (choice == 0)
            {
                //TODO - сделать выход
                return 0;
            }
            else
                selected_room = choice-1;
        }
        else
        {
            printf("%s", rooms[selected_room]);
            //Отображение истории сообщений
            get_new_messages_client(connection, selected_room, room_number[selected_room]);
            lseek(room_fd[selected_room], 0, SEEK_SET);
            read_messages(room_fd[selected_room]);
            //===Меню выбора действия===
            printf( WHITE BRIGHT "============================================================\n"
            DEFAULT "\tДействия в \"%s\"\n"
                    "1. Написать сообщение\n"
                    "2. Обновить\n"
            BRIGHT  "0. Выйти из комнаты\n"
            DEFAULT "Введите номер выбранного действия: ", rooms[selected_room]);
            char choice;
            
            choice = getchar();
            while (getchar() !='\n');   //Очистка буфера
            switch (choice)
            {
                case '0':
                selected_room = -1;
                clear()
                break;
                case '1':
                clear()
                get_new_messages_client(connection, selected_room, room_number[selected_room]);
                lseek(room_fd[selected_room], 0, SEEK_SET);
                read_messages(room_fd[selected_room]);
                printf( 
                WHITE BRIGHT "============================================================\n" //TODO - сделать возможность отмены отправки
                DEFAULT      "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
                WHITE BRIGHT "============================================================\n"
                DEFAULT);
                char buf[MAXBUFFER];
                fgets(buf, MAXBUFFER, stdin);
                send_message_client(connection, selected_room, nickname, buf);
                break;
                case '2':
                clear()
                continue;
                break; 
            }
        }
    }
    /*

    room_fd[0] = open("test_history", O_RDWR);
    room_number[0] = read_messages(room_fd[0]);
    
    lseek(room_fd[0], 0, SEEK_SET);
    check_connection(connection);
    
    get_name_client(connection);
    //send_message_client(sock, 0, "SmirnuX", "Hello, world");*/

    return 0;
}



int check_connection(struct s_connection* connection)
{
    char test[] = "/ping";
    send_message(connection->sock, test);
    fd_set connection_set;
    FD_ZERO(&connection_set);	//Обнуление набора
    FD_SET(connection->sock, &connection_set);
	struct timeval timeout;
	timeout.tv_usec = TIMEOUT_MS;
	timeout.tv_sec = 5;
    int a = select(connection->sock+1, &connection_set, NULL, NULL, &timeout);
    int b = 0;
	ioctl(connection->sock, FIONREAD, &b);
    if (a && !(b==0))
    {
        get_message(connection->sock, NULL);
        return 1;   //Соединение активно
    }
    //Попытка переподключиться
    printf("Соединению кранты\n");
    close(connection->sock);
    errno = 0;
    connection->sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(connection->sock, connection->address, sizeof(*(connection->address)));
    perror("Переподключение: ");
    return 0;

}

int get_rooms_client(struct s_connection* connection) //Получение списка комнат
{
    check_connection(connection);
    char buf[MAXBUFFER];
    strncpy(buf, "/getrooms", MAXBUFFER);
    send_message(connection->sock, buf);
    int count = atoi(get_message(connection->sock, buf));
    //TODO - сделать запись комнат и их количества в память, а также создание файлов историй
    for (int i=0; i<count; i++)
    {
        get_message(connection->sock, buf);        
        strncpy(rooms[i], buf, MAXNICKLEN);
    }
    room_count = count;
}
//TODO - добавить проверку времени сервера (т.е. при подключении сохраняется время запуска сервера, если сервер был закрыт и запущен заново - происходит заново получение списка комнат и сообщений)
int send_message_client(struct s_connection* connection, int room, char* nickname, char* message)    //Отправка сообщения серверу. args - вектор из трех строк - номера комнаты, никнейма и отправляемого сообщения
{
    check_connection(connection);
    char buf[MAXBUFFER];
    strncpy(buf, "/sendmessage", MAXBUFFER);
    send_message(connection->sock, buf);
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_message(connection->sock, buf);
    //Отправка никнейма
    send_message(connection->sock, nickname);
    //Отправка сообщения
    send_message(connection->sock, message);
}

int get_new_messages_client(struct s_connection* connection, int room, int count)  //Получение новых сообщений. args - вектор из двух строк - номера комнаты и количества уже имеющихся сообщений
{
    check_connection(connection);
    char buf[MAXBUFFER];   
    strncpy(buf, "/getnewmessages", MAXBUFFER);
    send_message(connection->sock, buf);
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_message(connection->sock, buf);
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", count);
    send_message(connection->sock, buf);
    //Прием количества сообщений
    int target_count = atoi(get_message(connection->sock, buf));
    //Прием сообщений
    for(int i = count; i<target_count; i++)
    {
        char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	    //Получение даты и времени
	    get_message(connection->sock, s_time);
    	//printf(DEFAULT BRIGHT"%s\n",s_time);
	    //Получение никнейма
	    get_message(connection->sock, nickname);
    	//printf(DEFAULT BRIGHT" %s\n", nickname);
	    //Получение сообщения
    	get_message(connection->sock, buf);
    	//printf(DEFAULT"%s\n\n", buf);
        //Запись сообщения в файл
    	write_message(room_fd[room], s_time, nickname, buf, ++room_number[room]);   
    }
}

char* get_name_client(struct s_connection* connection)  //Получение наименования сервера
{
    check_connection(connection);
    char buf[MAXBUFFER];
    strncpy(buf, "/getname", MAXBUFFER);
    send_message(connection->sock, buf);
    get_message(connection->sock, buf);
    char* res = malloc(sizeof(char)*MAXNICKLEN);
    strncpy(res, buf, MAXNICKLEN);
    return res;
}

int send_message(int socket, char* str) //Отправка произвольного сообщения
{
    ssize_t a = write(socket, str, strlen(str)+1);  
    return (int) a;
}

char* get_message(int socket, char* str)    //Прием произвольного сообщения
{
    int i, j;
    char temp;
    for (i=0; i<MAXBUFFER-1; i++)
    {  
        if (str == NULL)    //Прием сообщения без сохранения в памяти
        {
            j = read(socket, &temp, 1); //Считываем 
            if (j <= 0 || temp == '\0') 
                break;
        }
        else
        {
            j = read(socket, str+i, 1); 
            if (j <= 0) 
            {  
                str[i] = '\0';
                break;
            }
            if (str[i] == '\0')
                break;
        }
    }
    return str;
}