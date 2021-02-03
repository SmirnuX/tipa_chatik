// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int client(int sock, struct sockaddr* address)
{
    char* server_name;
    //Получение названия сервера
    server_name = get_name_client(sock);
    //Получение списка комнат
    //Проверка на подключение
    /*
    close(sock);
    errno = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, address, sizeof(*address));
    perror("connect: ");*/
    getc(stdin);
    check_connection(sock, address);
    return 1;
    get_rooms_client(sock);
    //Вывод списка комнат и выбор
    printf("\t\tСписок комнат сервера %s\n", server_name);
    //for(int i=0; i<room_count; i++)

    room_fd[0] = open("test_history", O_RDWR);
    room_number[0] = read_messages(room_fd[0]);
    get_new_messages_client(sock, 0, room_number[0]);
    lseek(room_fd[0], 0, SEEK_SET);
    read_messages(room_fd[0]);
    get_name_client(sock);
    //send_message_client(sock, 0, "SmirnuX", "Hello, world");

    return 0;
}

int check_connection(int sock, struct sockaddr* address)
{
    char test[] = "/ping";
    send_message(sock, test);
    fd_set connection;
    FD_ZERO(&connection);	//Обнуление набора
    FD_SET(sock, &connection);
	struct timeval timeout;
	timeout.tv_usec = TIMEOUT_MS;
	timeout.tv_sec = 5;
    int a = select(sock+1, &connection, NULL, NULL, &timeout);
    int b = 0;
	ioctl(sock, FIONREAD, &b);
    if (!a || (b==0))
    {
        printf("Соединению кранты\n");
    }
}

int get_rooms_client(int sock) //Получение списка комнат
{
    char buf[MAXBUFFER];
    strncpy(buf, "/getrooms", MAXBUFFER);
    send_message(sock, buf);
    int count = atoi(get_message(sock, buf));
    //TODO - сделать запись комнат и их количества в память, а также создание файлов историй
    for (int i=0; i<count; i++)
    {
        get_message(sock, buf);
        printf("%i. %s\n", i+1, buf);
    }
}
//TODO - добавить проверку времени сервера (т.е. при подключении сохраняется время запуска сервера, если сервер был закрыт и запущен заново - происходит заново получение списка комнат и сообщений)
int send_message_client(int sock, int room, char* nickname, char* message)    //Отправка сообщения серверу. args - вектор из трех строк - номера комнаты, никнейма и отправляемого сообщения
{
    char buf[MAXBUFFER];
    strncpy(buf, "/sendmessage", MAXBUFFER);
    send_message(sock, buf);
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_message(sock, buf);
    //Отправка никнейма
    send_message(sock, nickname);
    //Отправка сообщения
    send_message(sock, message);
}

int get_new_messages_client(int sock, int room, int count)  //Получение новых сообщений. args - вектор из двух строк - номера комнаты и количества уже имеющихся сообщений
{
    
    char buf[MAXBUFFER];   
    strncpy(buf, "/getnewmessages", MAXBUFFER);
    send_message(sock, buf);
    //Отправка комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_message(sock, buf);
    //Отправка количества сообщений
    snprintf(buf, MAXBUFFER, "%i", count);
    send_message(sock, buf);
    //Прием количества сообщений
    int target_count = atoi(get_message(sock, buf));
    //Прием сообщений
    for(int i = count; i<target_count; i++)
    {
        char s_time[MAXBUFFER], nickname[MAXBUFFER], buf[MAXBUFFER];
	    //Получение даты и времени
	    get_message(sock, s_time);
    	//printf(DEFAULT BRIGHT"%s\n",s_time);
	    //Получение никнейма
	    get_message(sock, nickname);
    	//printf(DEFAULT BRIGHT" %s\n", nickname);
	    //Получение сообщения
    	get_message(sock, buf);
    	//printf(DEFAULT"%s\n\n", buf);
        //Запись сообщения в файл
    	write_message(room_fd[room], s_time, nickname, buf, ++room_number[room]);   
    }
}

char* get_name_client(int sock)  //Получение наименования сервера
{
    char buf[MAXBUFFER];
    strncpy(buf, "/getname", MAXBUFFER);
    send_message(sock, buf);
    get_message(sock, buf);
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
    int i;
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