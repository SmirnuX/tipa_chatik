// SPDX-License-Identifier: CPOL-1.02
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

//Константы
#define PERMISSION 0666	//Разрешения для создаваемых файлов

#define MAXIPLEN 15 //Максимальная длина IP адреса
#define MAXPORTLEN 5    //Максимальная длина порта
#define MAXNICKLEN 64   //Максимальная длина никнейма
#define MAXBUFFER 256   //Максимальный размер буфера (также максимальная длина сообщения)
#define MAXROOMS 16 //Максимальное количество комнат (также максимальное количество открытых файлов)

//Файлы конфигурации
#define server_config "configs/serverconfig"
#define client_config "configs/clientconfig"

//escape-последовательности для изменения шрифта
#define RED "\033[31m"
#define BLUE "\033[34m"
#define WHITE "\033[37m"

#define DEFAULT "\033[0m"
#define BRIGHT "\033[1m"
#define DIM "\033[2m"
#define BLINK "\033[5m"

#define clear() printf("\033[H\033[J"); //очистка экрана



int client(int sock, char* nickname);
int server(int sock, char* nickname);

int get_string(char* buf, int maxlen, int fd);
int send_message(int socket, char* str);
char* get_message(int socket, char* str);

int get_rooms_client(int sock);
void get_rooms_server(int sock, int count, char** rooms);

int send_message_client(char* msg, int sock, char* nick);
int send_message_server(int sock);