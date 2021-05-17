#include "main.h"

void client_update(void* void_connection)    //Автоматическое обновление чата у клиента
{
    struct s_connection *connection = (struct s_connection*) void_connection; 
    listen(connection->sock, 1);
    while (1)
    {
        struct sockaddr server_address;
        struct sockaddr_in server_address;
        int len = sizeof(server_address);
        int server_socket = accept(connection->sock, (struct sockaddr*) &server_address, &len); //Получение информации о новом сообщении
        sem_wait(thread_lock);
        erase_lines(CLIENT_ACTION_MENU);
        //Сверяем версии и получаем номер комнаты
        char buf[MAXBUFFER];   
        if (atoi(get_data(connection->sock, buf)) != server_time) //Проверка "версии" сервера
        {
            send_data_safe(connection, "0");   //Информируем сервер о старой версии
            printf("Произошли изменения на сервере. Переподключение...\n\n");    //Печатаем уведомление о устарелости версии
            sel_room = -1;
        }
        send_data_safe(connection, "1");
        //Прием комнаты
        int room = atoi(get_data(connection->sock, buf));
        if (room == sel_room)   //Если находимся в той же комнате - печатаем полученное сообщение, иначе - печатаем уведомление
        {
            //Отправляем согласие на принятие сообщения
            send_data(connection->sock, "1");
            //Получаем сообщение
            char s_time[MAXBUFFER], nickname[MAXBUFFER];
            //Получение сообщения
            get_data(connection->sock, s_time);
            get_data(connection->sock, nickname);
            get_data(connection->sock, buf);
            //Запись сообщения в файл
            write_message(rooms[room]->fd, s_time, nickname, buf, ++(rooms[room]->msg_count)); 
            printf(DEFAULT BLUE"%s\n", s_time);
            printf(DEFAULT BRIGHT" %s\n", nickname);
            printf(DEFAULT"%s\n\n", buf);
        }
        else if (notifications_enable)
        {
            //Отклоняем сообщение
            send_data(connection->sock, "0");
            //Печатаем уведомление
            printf(CYAN "%s - Получено новое сообщение\n", rooms[room]->name);
        }
        print_menu(sel_room);
        sem_post(thread_lock);
    } 
    pthread_exit(NULL);
}

void server_send_update(int sock, int room, struct s_message* msg)   //Автоматическая отправка сообщений (или уведомлений)
{
    char buf[MAXBUFFER];	
    //Отправка номера комнаты
    snprintf(buf, MAXBUFFER, "%i", room);
    send_data(sock, buf);
    //Получение ответа
    if (atoi(get_data(sock, buf)) != 0)
    {
        //Передача сообщения
        strncpy(buf, msg->datetime, strlen(msg->datetime)+1);
        send_data(sock, buf);
        strncpy(buf, msg->nickname, strlen(msg->nickname)+1);
        send_data(sock, buf);
        strncpy(buf, msg->msg_text, strlen(msg->msg_text)+1);
        send_data(sock, buf);
    }
	return 0;
}