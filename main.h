// SPDX-License-Identifier: CPOL-1.02
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
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
#define CMD_COUNT 4 //Количество команд, осуществляющих интерфейс клиент-сервер
#define MAXCONNECTIONS 5    //Количество соединений, которые могут быть открыты одновременно
#define MAXQUEUE 5  //Максимальное количество соединений в очереди
#define TIMEOUT_S 0
#define TIMEOUT_MS 400 //Частота опроса сокета в микросекундах

//Файлы конфигурации
#define server_config "configs/serverconfig"
#define client_config "configs/clientconfig"

//escape-последовательности для управления выводом
#define RED "\033[31m"
#define BLUE "\033[34m"
#define WHITE "\033[37m"

#define DEFAULT "\033[0m"
#define BRIGHT "\033[1m"
#define DIM "\033[2m"
#define BLINK "\033[5m"

#define clear() printf("\033[H\033[J"); //очистка экрана
#define prev_line() printf("\033[1A"); //Поднять курсор на строку выше
#define erase_line() printf("\r\033[K\r");  //Стереть строку

//Количество строк в меню:
#define SERVER_MENU_LINES 7   
#define SERVER_NEW_ROOM_LINES 5
#define SERVER_CHOOSE_ROOM_LINES 6
#define SERVER_WRITE_MESSAGE_LINES 4




//Структура сообщения
struct s_message
{
    char datetime[MAXBUFFER];
    char nickname[MAXNICKLEN];
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
extern long server_time;    //Время запуска сервера



//Команды, выполняемые на сервере
extern const char *server_cmd_strings[CMD_COUNT];  //Список названий команд
extern int (*server_cmd_functions[CMD_COUNT])(struct s_connection* connection);  //Соответствующие им функции

int get_rooms_client(struct s_connection* connection);    //Получение списка комнат через сокет sock
int get_rooms_server(struct s_connection* connection);	//Отправка списка комнат через сокет sock

int send_message_client(struct s_connection* connection, int room, char* nickname, char* message); //Отправка сообщения серверу. 
int send_message_server(struct s_connection* connection); //Получение сообщения сервером. Аргумент args не используется

int get_new_messages_client(struct s_connection* connection, int room, int count); //Получение недостающих сообщений.
int get_new_messages_server(struct s_connection* connection);	//Отправка недостаюших сообщений клиенту. 

char* get_name_client(struct s_connection* connection);    //Получение наименования сервера
int get_name_server(struct s_connection* connection); //Отправка наименования сервера клиенты


int get_string(char* buf, int maxlen, int fd);

int client_send_message(struct s_connection* connection, char* str); //Отправка произвольного сообщения
int server_send_message(struct s_connection* connection, char* str);
char* get_message(int socket, char* str);
int read_messages(int room);	//Вывод всех сообщений, начиная с установленной ранее позици в файле
int read_single_message(int room, struct s_message* msg);
int write_message(int room, char* datetime, char* nickname, char* msg, int number); //Запись сообщения в файл
int goto_message(int room, int count);  //Перемещение позиции в файле к count сообщению с конца
int count_messages(int room);
int send_message(struct s_connection* connection, char* str); //Отправка произвольного сообщения

void erase_lines(int n);

void rooms_sort();	//Сортировка списка комнат
void rooms_swap(int a, int b);

void remove_new_line(char* str) //Убирает последний символ новой строки (если есть)