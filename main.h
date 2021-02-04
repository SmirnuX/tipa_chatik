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
#define SIZEOF_MAXLENGTH_FORMAT "%08i"  //Формат для printf. 
#define CMD_COUNT 5 //Количество команд, осуществляющих интерфейс клиент-сервер
#define MAXCONNECTIONS 5    //Количество соединений, которые могут быть открыты одновременно
#define MAXQUEUE 5  //Максимальное количество соединений в очереди
#define TIMEOUT_S 0
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

//Структура сообщения
struct s_message
{
    char datetime[MAXBUFFER];
    char nickname[MAXBUFFER];   //TODO - динамически
    char msg_text[MAXBUFFER];
};

//Структура соединения
struct s_connection
{
    int sock;   //Сокет подключения
    struct sockaddr* address;   //Адрес (используется для переподключения)
};

int client(struct s_connection* connection);
int server(struct s_connection* connection);

//Сведения о комнатах
extern char* rooms[MAXROOMS];	//Названия комнат
extern int room_fd[MAXROOMS];	//Файловые дескрипторы комнат
extern int room_number[MAXROOMS];	//Количество сообщений в комнатах
extern int room_count;	//Количество комнат
extern char nickname[MAXNICKLEN];



//Команды, выполняемые на сервере
extern const char *server_cmd_strings[CMD_COUNT];  //Список названий команд
extern int (*server_cmd_functions[CMD_COUNT])(int);  //Соответствующие им функции

int get_rooms_client(struct s_connection* connection);    //Получение списка комнат через сокет sock
int get_rooms_server(int sock);	//Отправка списка комнат через сокет sock

int send_message_client(struct s_connection* connection, int room, char* nickname, char* message); //Отправка сообщения серверу. 
int send_message_server(int sock); //Получение сообщения сервером. Аргумент args не используется

int get_new_messages_client(struct s_connection* connection, int room, int count); //Получение недостающих сообщений.
int get_new_messages_server(int sock);	//Отправка недостаюших сообщений клиенту. 

char* get_name_client(struct s_connection* connection);    //Получение наименования сервера
int get_name_server(int sock); //Отправка наименования сервера клиенты

int ping_server(int sock);

int get_string(char* buf, int maxlen, int fd);
int send_message(int socket, char* str);
char* get_message(int socket, char* str);
int read_messages(int room);	//Вывод всех сообщений, начиная с установленной ранее позици в файле
int read_single_message(int room, struct s_message* msg);
int write_message(int room, char* datetime, char* nickname, char* msg, int number); //Запись сообщения в файл
int goto_message(int room, int count);  //Перемещение позиции в файле к count сообщению с конца

int check_connection(struct s_connection* connection);