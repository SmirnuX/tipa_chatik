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
#define FOLDERPERMISSION 0777   //Разрешения для создаваемых папок
#define MAXIPLEN 15 //Максимальная длина IP адреса
#define MAXPORTLEN 5    //Максимальная длина порта
#define MAXNICKLEN 64   //Максимальная длина никнейма
#define MAXBUFFER 256   //Максимальный размер буфера (также максимальная длина сообщения)
#define MAXROOMS 16 //Максимальное количество комнат (также максимальное количество открытых файлов)
#define MAXFILES 16 //Максимальное количество файлов в комнате
#define SIZEOF_MAXLENGTH 8 //Длина позиции в файле (т.е. максимальная длина файла - 10^16б > 95 Мб)
#define SIZEOF_MAXLENGTH_FORMAT "%08i"  //Формат для printf. 
#define CMD_COUNT 7 //Количество команд, осуществляющих интерфейс клиент-сервер
#define MAXCONNECTIONS 5    //Количество соединений, которые могут быть открыты одновременно
#define MAXQUEUE 5  //Максимальное количество соединений в очереди
#define TIMEOUT_S 0
#define TIMEOUT_MS 400 //Частота опроса сокета в микросекундах

//Файлы конфигурации
#define server_config "configs/serverconfig"
#define client_config "configs/clientconfig"

//escape-последовательности для управления выводом
#define RED "\033[31m"
#define GREEN "\033[32m"
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
#define FILESPERPAGE 10 //Количество файлов на одной странице

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

//Структура комнаты
struct s_room
{
    char* name;    //Название комнаты
    int fd;    //Файловый дескриптор файла истории
    int msg_count;  //Количество сообщений
    int file_count;    //Количество файлов
    char* file_names[MAXFILES]; //Наименования файлов
};

//Основные функции
int client(struct s_connection* connection);    
int server(struct s_connection* connection);

//Сведения о комнатах
extern struct s_room* rooms[MAXROOMS];	//Комнаты
extern int room_count;	//Количество комнат
extern char nickname[MAXNICKLEN];   //Никнейм пользователя или название сервера
extern long server_time;    //Время запуска сервера

//Команды, выполняемые на сервере
extern const char *server_cmd_strings[CMD_COUNT];  //Список команд
extern int (*server_cmd_functions[CMD_COUNT])(int sock);  //Соответствующие им функции

//Клиентская часть команд
int get_rooms_client(struct s_connection* connection);    //Получение списка комнат от сервера через соединение connection. В процессе выделяется память, которая должна быть очищена! (rooms[i])
int send_message_client(struct s_connection* connection, int room, char* nickname, char* message);  //Отправка сообщения message на сервер через соединение connection в комнату под номером room пользователем nickname
int get_new_messages_client(struct s_connection* connection, int room, int count); //Получение недостающих сообщений через соединение connection для комнаты под номером room, в которой на данный момент сохранено count сообщений
char* get_name_client(struct s_connection* connection);    //Получение наименования сервера через соединение connection
int get_files_client(struct s_connection* connection, int room, int page);  //Получение списка файлов комнаты
int download_file_client(struct s_connection* connection, int room, int number);
int send_file_client(struct s_connection* connection, int room, int file, char* filename);

//Серверная часть команд
int get_rooms_server(int sock);	//Отправка списка комнат через сокет sock
int send_message_server(int sock); //Получение сообщения сервером через сокет sock
int get_new_messages_server(int sock);	//Отправка недостаюших сообщений клиенту через сокет sock
int get_name_server(int sock); //Отправка наименования сервера клиенту через сокет sock
int get_files_server(int sock);
int download_file_server(int sock);
int send_file_server(int sock);

//Функции для обмена произвольными сообщениями между клиентом и сервером
int send_data_safe(struct s_connection* connection, char* str); //Отправка произвольного сообщения str с попыткой переподключения в случае неудачи
int send_ndata_safe(struct s_connection* connection, char* str, int n);  //Отправка сообщения str длиной n с попыткой переподключения в случае неудачи
int send_data(int sock, char* str);  //Отправка произвольного сообщения str БЕЗ попытки переподключения
char* get_data(int socket, char* str);   //Прием произвольного сообщения
char* get_ndata(int socket, char* str, int n);  //Прием сообщения размером n. В случае неудачи записывает в конце принятой части сообщения \0

//Функции для работы с сообщениями, сохраненными в файле
int get_string(char* buf, int maxlen, int fd);  //Получение строки из файла. При возникновении ошибки возвращает -1
int read_messages(int room);	//Вывод всех сообщений из файла, соответствующего комнате под номером room, начиная с установленной ранее позиции в файле
int read_single_message(int room, struct s_message* msg);   //Чтение одного сообщения из файла, соответствующего комнате под номером room, начиная с установленной ранее позиции в файле. Чтение производится в структуру msg
int write_message(int room, char* datetime, char* nickname, char* msg, int number); //Запись сообщения в файл, соответствующий комнате под номером room
int goto_message(int room, int count);  //Перемещение позиции в файле, соответствующему комнате под номером room к count сообщению с конца
int count_messages(int room);   //Подсчет количества сообщению в файле, соответствующему комнате под номером room, начиная с установленной ранее позиции в файле.
/*	Хранение истории происходит в следующем формате:
	<1> - номер сообщения
	<01/02/2003> - дата и время отправления
	<Alice> - никнейм отправителя
	<Hello, world> - сообщение
	<00000000> - указывает на предыдущую позицию (т.е. перед номером текущего сообщения). Длина позиции определяется макросом SIZEOF_MAXLENGTH */

//Функции для управления списком комнат
void rooms_sort();	//Сортировка списка комнат
void rooms_swap(int a, int b);  //Обмен комнат под номерами a и b местами

//Вспомогательные функции
void remove_new_line(char* str); //Убирает последний символ новой строки (если есть)
void erase_lines(int n);    //Перемещение курсора на n строк наверх (и удаление этих строк)

//Функции интерфейса
int client_ui_download_files(struct s_connection* connection, int* selected_room);