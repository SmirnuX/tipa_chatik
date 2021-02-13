// SPDX-License-Identifier: CPOL-1.02З
#include "main.h"

const char *server_cmd_strings[CMD_COUNT] = { //Команды, выполняемые на сервере
    "/getrooms",    //Получение списка комнат.      Синтаксис: /getrooms.
    "/sendmessage", //Отправка сообщения на сервер. Синтаксис: /sendmessage <room> <nickname> <message>
    "/getnewmessages",  //Получение новых сообщений с сервера. Синтаксис: /getnewmessages <room> <number> (number - количество уже имеющихся сообщений)
    "/getname",  //Получение наименования сервера
};  //Команды для взаимодействия клиент - сервер
int (*server_cmd_functions[CMD_COUNT])(int sock) = {  //Соответствующие им функции сервера
    get_rooms_server,
    send_message_server,
    get_new_messages_server,
    get_name_server,
};
long server_time;    //Время запуска сервера

char* rooms[MAXROOMS];	//Названия комнат
int room_fd[MAXROOMS];	//Файловые дескрипторы комнат
int room_number[MAXROOMS];	//Количество сообщений в комнатах
int room_count;	//Количество комнат
char nickname[MAXNICKLEN];  //Наименование клиента/сервера


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
    int running = 1;
    while (running)
    {
        if (!edit_config)
        {
            printf("\tЗагрузка файла конфигурации...\n");
            config = open(config_file, O_RDONLY);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (config == -1)
            {
                if (errno == ENOENT)
                {
                    printf( 
                    RED BRIGHT "Файл конфигурации не найден\n" WHITE DEFAULT);
                    edit_config = 1;
                    errno = 0;
                }
                else
                {
                    perror("Ошибка открытия файла конфигурации: ");
                    return 1;
                }
            }
            else    //Чтение из уже имеющегося файла конфигурация
            {
                
                if (get_string(ipaddr, MAXIPLEN, config) == -1 || 
                get_string(port_str, MAXPORTLEN, config) == -1 || 
                get_string(nickname, MAXNICKLEN, config) == -1)
                {
                    printf( 
                    RED BRIGHT "Файл конфигурации имеет неправильный формат\n" WHITE DEFAULT);
                    edit_config = 1;
                } 
            }   
        }
        if (edit_config)    //Создание/редактирования файла конфигурации
        {
            printf("\tВведите IP адрес, по которому будет производиться подключение\n>");    
            __fpurge(stdin);  //Очистка буфера        
            if (fgets(ipaddr, MAXIPLEN, stdin) == NULL)
            {
                printf ("Ошибка ввода IP адреса.\n\n");
                continue;
            }
            remove_new_line(ipaddr);
            __fpurge(stdin);  //Очистка буфера
            printf("\tВведите порт, по которому будет производиться подключение\n>");
            if (fgets(port_str, MAXPORTLEN, stdin) == NULL)
            {
                printf ("Ошибка ввода порта.\n\n");
                continue;
            }
            remove_new_line(port_str);
            __fpurge(stdin);  //Очистка буфера
            if (is_server)
                printf("\tВведите наименование сервера.\n>");
            else
                printf("\tВведите никнейм.\n>");
            if (fgets(nickname, MAXNICKLEN, stdin) == NULL)
            {
                printf ("Ошибка ввода никнейма.\n\n");
                continue;
            }
            remove_new_line(nickname);
        }
        port = atoi(port_str);
        //Проверка введенных данных на корректность
        if (inet_aton(ipaddr, &address.sin_addr) == 0)
        {
            printf ("Неправильный формат IP адреса!\n\n");
            edit_config = 1;
            continue;
        }
        if (port < 0 || port > 65535)
        {
            printf ("Неправильный формат порта!\n\n");
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
                perror("Ошибка создания файла конфигурации: ");
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
        int respond;

        if (is_server == 0)
            respond = connect(sock, (struct sockaddr *) &address, sizeof(address));
        else
        {
            fcntl(sock, F_SETFL, O_NONBLOCK);
            respond = bind(sock, (struct sockaddr *) &address, sizeof(address));
        }
        struct s_connection connection;
        connection.sock = sock;
        connection.address = (struct sockaddr *) &address;


        if (respond == -1)
        {
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
                client(&connection);
            else
                server(&connection);
            running = 0;
        }  
    }
    //TODO - вставить очистку памяти
    return 0;
}

int get_string(char *buf, int maxlen, int fd)   //Получение строки из файла. При возникновении ошибки возвращает -1
{
    int i;
    for (i = 0; i < maxlen + 1; i++)
    {
        switch (read(fd, buf + i, 1))
        {
            case -1:
                perror("Ошибка файла конфигурации: ");
                return -1;
                break;
            case 0:
                buf[i] = '\n';
                break;
        }
        if (buf[i] == '\n')
        {
            buf[i] = '\0';
            break;
        }
        else if (i == maxlen)
        {
            printf("Ошибка файла конфигурации: неправильный формат.\n");
            return -1;
        }
    }
    return i;
}

int goto_message(int room, int count)	//Перемещение позиции в файле к count сообщению с конца
{
	char buf[MAXBUFFER];
    long pos;
	lseek(room, -SIZEOF_MAXLENGTH, SEEK_END); //Чтение позиции последнего сообщения
	for (int i = 0; i < count; i++)
	{
		read(room, buf, SIZEOF_MAXLENGTH);
		buf[SIZEOF_MAXLENGTH] = '\0';
		int offset = atoi(buf);
		if (offset < 0)
			offset = 0;
		pos = lseek(room, offset, SEEK_SET);
        if (pos < 0)
            break;
	}
    if (pos > 0)
	    lseek(room, SIZEOF_MAXLENGTH, SEEK_CUR);
}

int read_messages(int room)	//Вывод всех сообщений из файла, соответствующего комнате под номером room, начиная с установленной ранее позиции в файле
{
    struct s_message message;
    int i;
	for (i = 0; read_single_message(room, &message) != 1; i++)
	{	  
        printf(DIM BLUE"%s\n", message.datetime);
        printf(DEFAULT BRIGHT" %s\n", message.nickname);
        printf(DEFAULT"%s\n\n", message.msg_text);	
	}	
    return i;
}

int count_messages(int room)	//Подсчет количества сообщению в файле, соответствующему комнате под номером room, начиная с установленной ранее позиции в файле.
{
    struct s_message message;
    int i;
	for (i = 0; read_single_message(room, &message) != 1; i++)
	{	  

	}	
    return i;
}

int read_single_message(int room, struct s_message* msg)    //Чтение одного сообщения из файла, соответствующего комнате под номером room, начиная с установленной ранее позиции в файле. Чтение производится в структуру msg
{
    char buf[MAXBUFFER];
	int size;
    for (int j = 0; j < 4; j++)	//Чтение сообщений состоит из четырех частей
    {
        buf[MAXBUFFER-1] = '\0';
        for (size = 0; size < MAXBUFFER-1; size++)
        {
            if (read(room, buf+size, 1) != 1)
            {
                return 1;
            }
            if (buf[size] == '\0')
                break;
        }
        switch (j)
        {
            case 1:
            strncpy(msg->datetime, buf, strlen(buf)+1);
            break;
            case 2:
            strncpy(msg->nickname, buf, strlen(buf)+1);
            break;
            case 3:
            strncpy(msg->msg_text, buf, strlen(buf)+1);
            break;
        }
    }
    lseek(room, SIZEOF_MAXLENGTH, SEEK_CUR);    //Пропуск позиции
    return 0;
}

int write_message(int room, char* datetime, char* nickname, char* msg, int number)	//Запись сообщения в файл, соответствующий комнате под номером room
{
	//Запись номера
	char buf[MAXBUFFER];
	int pos = lseek(room, 0, SEEK_END);
	snprintf(buf, MAXBUFFER, "%i", number);
	write(room, buf, strlen(buf)+1);
	write(room, datetime, strlen(datetime)+1);
	write(room, nickname, strlen(nickname)+1);
	write(room, msg, strlen(msg)+1);
	snprintf(buf, MAXBUFFER, SIZEOF_MAXLENGTH_FORMAT, pos - SIZEOF_MAXLENGTH);
	write(room, buf, SIZEOF_MAXLENGTH);
}

void remove_new_line(char* str) //Убирает последний символ новой строки (если есть)
{
    if (str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';
}