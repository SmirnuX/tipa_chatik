// SPDX-License-Identifier: CPOL-1.02
#include "main.h"

int client(int sock, char* nickname)
{
    char buf[MAXBUFFER];
    clear()
    printf("Выберите комнату из списка: \n");
    get_rooms_client(sock);
    send_message_client("Hello, world", sock, nickname);
    close(sock);
    return 0;
}

int send_message(int socket, char* str)
{
    write(socket, str, strlen(str)+1);
}
    /*  Список команд
        /getrooms +
        /getmessages <room> <count>
        /sendmessage <room> <nickname> <datetime> <text>

        Сообщение:
        <msg><Позиция в файле начала предыдущего сообщения><Никнейм отправителя><Дата отправления><Сообщение>
    */

char* get_message(int socket, char* str)
{
    int i;
/*
    while (read(socket, str, 1)!=1)
    {

    }*/

    for (i=0; i<MAXBUFFER-1; i++)
    {  
        int j = read(socket, str+i, 1);
        //printf("%i\n", j);
        if (j!=1) 
        {  
            str[i] = '\0';
            break;
        }
        if (str[i] == '\0')
            break;
    }
    return str;
}

int get_rooms_client(int sock) //Получение списка комнат
{
    char buf[MAXBUFFER];
    strncpy(buf, "/getrooms", MAXBUFFER);
    send_message(sock, buf);
    int count = atoi(get_message(sock, buf));
    for (int i=0; i<count; i++)
    {
        get_message(sock, buf);
        printf("%i. %s\n", i+1, buf);
    }
}

int send_message_client(char* msg, int sock, char* nick)
{
    char buf[MAXBUFFER];
    strncpy(buf, "/sendmessage", MAXBUFFER);
    send_message(sock, buf);
    //Отправка никнейма
    strncpy(buf, nick, MAXNICKLEN);
    send_message(sock, buf);
    //Отправка сообщения
    strncpy(buf, msg, MAXBUFFER);
    send_message(sock, buf);
}