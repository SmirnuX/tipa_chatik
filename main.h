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
#include <sys/ioctl.h>

//Константы
#define PERMISSION 0666	//Разрешения для создаваемых файлов

#define MAXIPLEN 15 //Максимальная длина IP адреса
#define MAXPORTLEN 5    //Максимальная длина порта
#define MAXNICKLEN 64   //Максимальная длина никнейма
#define MAXBUFFER 256   //Максимальный размер буфера (также максимальная длина сообщения)
#define MAXROOMS 16 //Максимальное количество комнат (также максимальное количество открытых файлов)
#define SIZEOF_MAXLENGTH 8 //Длина позиции в файле (т.е. максимальная длина файла - 10^16б > 95 Мб)
#define CMD_COUNT 5 //Количество команд, осуществляющих интерфейс клиент-сервер
#define MAXCONNECTIONS 5    //Количество соединений, которые могут быть открыты одновременно
#define MAXQUEUE 5  //Максимальное количество соединений в очереди
#define TIMEOUT_MS 400 //Частота опроса сокета в микросекундах

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

int client(int sock, struct sockaddr* address);
int server(int sock, struct sockaddr* address);

//Сведения о комнатах
extern char* rooms[MAXROOMS];	//Названия комнат
extern int room_fd[MAXROOMS];	//Файловые дескрипторы комнат
extern int room_number[MAXROOMS];	//Количество сообщений в комнатах
extern int room_count;	//Количество комнат
extern char nickname[MAXNICKLEN];

//Структура сообщения
struct message
{
    char datetime[MAXBUFFER];
    char nickname[MAXBUFFER];   //TODO - динамически
    char msg_text[MAXBUFFER];
};

//Команды, выполняемые на сервере
extern const char *server_cmd_strings[CMD_COUNT];  //Список названий команд
extern int (*server_cmd_functions[CMD_COUNT])(int, char**);  //Соответствующие им функции

int get_rooms_client(int sock);    //Получение списка комнат через сокет sock
int get_rooms_server(int sock, char** args);	//Отправка списка комнат через сокет sock

int send_message_client(int sock, int room, char* nickname, char* message); //Отправка сообщения серверу. 
int send_message_server(int sock, char** args); //Получение сообщения сервером. Аргумент args не используется

int get_new_messages_client(int sock, int room, int count); //Получение недостающих сообщений.
int get_new_messages_server(int sock, char** args);	//Отправка недостаюших сообщений клиенту. 

char* get_name_client(int sock);    //Получение наименования сервера
int get_name_server(int sock, char** args); //Отправка наименования сервера клиенты

int ping_server(int sock, char** args);

int get_string(char* buf, int maxlen, int fd);
int send_message(int socket, char* str);
char* get_message(int socket, char* str);
int read_messages(int room);	//Вывод всех сообщений, начиная с установленной ранее позици в файле
int read_single_message(int room, struct message* msg);
int write_message(int room, char* datetime, char* nickname, char* msg, int number); //Запись сообщения в файл
int goto_message(int room, int count);  //Перемещение позиции в файле к count сообщению с конца

int check_connection(int sock, struct sockaddr* address);