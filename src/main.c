// SPDX-License-Identifier: CPOL-1.02З
#include "main.h"

const char *server_cmd_strings[CMD_COUNT] = { //Команды, выполняемые на сервере
	"/getrooms",    //Получение списка комнат.      Синтаксис: /getrooms.
	"/sendmessage", //Отправка сообщения на сервер. Синтаксис: /sendmessage <room> <nickname> <message>
	"/getnewmessages",  //Получение новых сообщений с сервера. Синтаксис: /getnewmessages <room> <number> (number - количество уже имеющихся сообщений)
	"/getname",  //Получение наименования сервера
    "/getfiles",    //Получение количества файлов и названий десяти файлов. Синтаксис: /getfiles <room> <pagenumber> (pagenumber - номер страницы из 10 файлов)
    "/downloadfile",   //Загрузка файла с сервера. Синтаксис: /downloadfile <room> <number>
    "/sendfile" //Загрузка файла на сервер. Синтаксис: /sendfile <room> <filename> <filesize>
};  //Команды для взаимодействия клиент - сервер
int (*server_cmd_functions[CMD_COUNT])(int sock) = {  //Соответствующие им функции сервера
	get_rooms_server,
	send_message_server,
	get_new_messages_server,
	get_name_server,
    get_files_server,
    download_file_server,
    send_file_server
};
long server_time;    //Время запуска сервера

struct s_room* rooms[MAXROOMS];
int room_count;	//Количество комнат
char nickname[MAXNICKLEN];  //Наименование клиента/сервера
sem_t* thread_lock; //Семафор, показывающий, происходит ли изменение интерфейся другим потоком
enum MENUS menu;   //Количество строк в текущем меню
char notifications_enable;   //Включены ли уведомления о новых сообщениях в других комнатах
int sel_room;   //Выбранная в данный момент комната

int main(int argc, char* argv[])
{
    char* config_file;
    short is_server; 
    char ipaddr[MAXIPLEN+1];
    char port_str[MAXPORTLEN+1];
    int port;
    struct sockaddr_in address;

    if (argc == 2 && strcmp(argv[1], "-s")==0)
    {
        config_file = server_config;
        is_server = 1;
        printf("___TIPA SERVER___\n\n");
    }
    else
    {
        config_file = client_config;
        is_server = 0;
        printf("***TIPA CHATIK***\n\n");
    }
    int edit_config = 0;
    int config;
    int sock;
    int auto_sock;
    int running = 1;
	while (running == 1)	{
        if (!edit_config)
        {
            printf("\tЗагрузка файла конфигурации...\n");
            config = open(config_file, O_RDONLY);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (config == -1)
            {
                if (errno == ENOENT)
                {
                    ui_show_error("Файл конфигурации не найден.", 1);
                    edit_config = 1;
                    errno = 0;
                }
                else
                {
                    ui_show_error("Ошибка открытия файла конфигурации.", 1);
                    close(sock);
                    return 1;
                }
            }
            else    //Чтение из уже имеющегося файла конфигурация
            {
                
                if (get_string(ipaddr, MAXIPLEN, config) == -1 || 
                get_string(port_str, MAXPORTLEN, config) == -1 || 
                get_string(nickname, MAXNICKLEN, config) == -1)
                {
                    ui_show_error("Файл конфигурации имеет неправильный формат.", 0);
                    edit_config = 1;
                } 
            }   
        }
        if (edit_config)    //Создание/редактирования файла конфигурации
        {
            printf("\tВведите IP адрес, по которому будет производиться подключение\n>");           
            if (tipa_gets(ipaddr, MAXIPLEN, STDIN_FILENO) == NULL)
            {
                ui_show_error("Ошибка ввода IP адреса.", 0);
                continue;
            }
            remove_new_line(ipaddr);
            printf("\tВведите порт, по которому будет производиться подключение\n>");
            if (tipa_gets(port_str, MAXPORTLEN, STDIN_FILENO) == NULL)
            {
                ui_show_error("Ошибка ввода порта.", 0);
                continue;
            }
            remove_new_line(port_str);
            if (is_server)
                printf("\tВведите наименование сервера.\n>");
            else
                printf("\tВведите никнейм.\n>");
            if (tipa_gets(nickname, MAXNICKLEN, STDIN_FILENO) == NULL)
            {
                ui_show_error("Ошибка ввода никнейма.", 0);
                continue;
            }
            remove_new_line(nickname);
        }
        port = atoi(port_str);
        //Проверка введенных данных на корректность
        if (inet_aton(ipaddr, &address.sin_addr) == 0)
        {
            ui_show_error("Неправильный формат IP адреса.", 0);
            edit_config = 1;
            continue;
        }
        if (port < 0 || port > 65535)
        {
            ui_show_error("Неправильный формат порта.", 0);
            edit_config = 1;
            continue;
        }
        close(config);
        //Сохранение данных в файле конфигурации
        if (edit_config)
        {
            remove(config_file);
            config = open(config_file, O_CREAT | O_WRONLY, PERMISSION);
            if (config < 0)
            {
                ui_show_error("Ошибка создания файла конфигурации.", 1);
                return 1;
            }
            //Запись строк в файл
            write(config, ipaddr, strlen(ipaddr));
            write(config, "\n", sizeof(char));
            write(config, port_str, strlen(port_str));
            write(config, "\n", sizeof(char));
            write(config, nickname, strlen(nickname));
            close(config);
            edit_config = 0;
        }
        printf("Будет использована следующая конфигурация:\n"
            "\t IP сервера: \t%s:%i\n"
            "\t Никнейм: \t%s\n"
            , ipaddr, port, nickname); 
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ipaddr);
        address.sin_port = htons(port);
        struct sockaddr_in auto_address;
        auto_address.sin_family = AF_INET;
        auto_address.sin_addr.s_addr = address.sin_addr.s_addr;
        auto_address.sin_port = htons(port+1);  //TODO - добавить в описание, что должны быть свободны ДВА порта.
        int respond;
        struct s_connection connection;
        struct s_connection auto_connection;
        if (is_server == 0)
        {
            respond = connect(sock, (struct sockaddr *) &address, sizeof(address));
            respond |= connect(auto_sock, (struct sockaddr *) &auto_address, sizeof(auto_address));
            auto_connection.sock = auto_sock;
            auto_connection.address = (struct sockaddr *) &auto_address;
        }
        else
        {
            fcntl(sock, F_SETFL, O_NONBLOCK);
            respond = bind(sock, (struct sockaddr *) &address, sizeof(address));
            respond |= bind(auto_sock, (struct sockaddr *) &auto_address, sizoef(auto_address));
        }
        connection.sock = sock;
        connection.address = (struct sockaddr *) &address;

        if (respond == -1)
        {
            close(sock);
            close(auto_sock);
            perror( BRIGHT RED      "Ошибка подключения: ");
            printf( DEFAULT WHITE   "Выберите действие\n"
                            "1. Повторить попытку подключения\n"
                            "2. Изменить конфигурацию\n"
                            "0. Закрыть программу\n");
            switch (getchar())
            {
                case '0':
                running = 0;
                break;
                case '2':
                edit_config = 1;
            }
            if (!running)
                break;
            while (getchar() != '\n');   //Очистка буфера
            clear()
            errno = 0;
        }
        else
        {  
            if (is_server == 0)
            {
                sem_init(thread_lock, 0, 0);    //TODO - ошибки. Инициализация семафора
                sel_room = -1;
                notifications_enable = 1;
                pthread_t update_thread;
                pthread_create(update_thread, NULL, client_update, (void*) &auto_connection);
                client(&connection);
                pthread_cancel(update_thread);
                close(auto_connection.sock);
            }
            else
                server(&connection, &auto_connection);
            running = 0;
        }         
    }
    return 0;
}