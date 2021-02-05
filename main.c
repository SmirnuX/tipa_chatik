// SPDX-License-Identifier: CPOL-1.02З
#include "main.h"

const char *server_cmd_strings[CMD_COUNT] = { //Команды, выполняемые на сервере
    "/getrooms",    //Получение списка комнат.      Синтаксис: /getrooms.
    "/sendmessage", //Отправка сообщения на сервер. Синтаксис: /sendmessage <room> <nickname> <message>
    "/getnewmessages",  //Получение новых сообщений с сервера. Синтаксис: /getnewmessages <room> <number> (number - количество уже имеющихся сообщений)
    "/getname",  //Получение наименования сервера
    "/ping" //Команда для проверки соединения. Должна на ответ подать "pong"
};  //Команды для взаимодействия клиент - сервер
int (*server_cmd_functions[CMD_COUNT])(int) = {  //Соответствующие им функции сервера
    get_rooms_server,
    send_message_server,
    get_new_messages_server,
    get_name_server,
    ping_server
};

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
    int port;

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
    while (1)
    {
        printf("\tЗагрузка файла конфигурации...\n");
        int config = open(config_file, O_RDONLY);
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address;
        if (config == -1 && errno == ENOENT)    //Создание нового файла конфигурации
        {
            printf( 
            RED BRIGHT "Файл конфигурации не найден\n"
            WHITE DEFAULT "Создание файла конфигурации...\n>");
            //TODO Создание нового файла конфигурации
        }
        else
        {
            char port_str[MAXPORTLEN+1];
            if (get_string(ipaddr, MAXIPLEN, config) == -1 || 
            get_string(port_str, MAXPORTLEN, config) == -1 || 
            get_string(nickname, MAXNICKLEN, config) == -1)
            {
                //TODO Создание нового файла конфигурации
                continue;
            }
            port = atoi(port_str);
            printf("Будет использована следующая конфигурация:\n"
            "\t IP сервера: \t%s:%i\n"
            "\t Никнейм: \t%s\n"
            , ipaddr, port, nickname);
        }
        //TODO - добавить проверку для IP и порта
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
            perror("Ошибка подключения: ");
            printf(BLINK "Нажмите пробел для повторной попытки, или другую клавишу для выхода." DEFAULT);
            if (getc(stdin) != ' ')
                return 0;
            clear()
            errno = 0;
        } 

        if (is_server == 0)
            client(&connection);
        else
            server(&connection);
    
        break;    
    }
    return 0;
}

int get_string(char *buf, int maxlen, int fd)
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
	lseek(room, -SIZEOF_MAXLENGTH, SEEK_END); //Чтение позиции предыдущего сообщения
	for (int i = 0; i < count; i++) 	//TODO - добавить проверку на выход за файл
	{
		read(room, buf, SIZEOF_MAXLENGTH);
		buf[SIZEOF_MAXLENGTH] = '\0';
		//printf("%s, ", buf);
		int offset = atoi(buf);
		if (offset < 0)
			offset = 0;
		pos = lseek(room, offset, SEEK_SET);
	}
    if (pos > 0)
	    lseek(room, SIZEOF_MAXLENGTH, SEEK_CUR);
}

int read_messages(int room)	//Вывод всех сообщений (ВНИМАНИЕ: смещение в файле не меняется, перед чтением позиция должна находиться либо в начале файла, либо после позиции сообщения, предшествующего первому считываемому)
{
    struct s_message message;
    int i;
	for (i = 0; read_single_message(room, &message) != 1; i++)
	{	  
        printf(DIM WHITE"%s\n", message.datetime);
        printf(DEFAULT BRIGHT" %s\n", message.nickname);
        printf(DEFAULT"%s\n\n", message.msg_text);	
	}	
    return i;
}

int count_messages(int room)	//Подсчет количества сообщений в файле (ВНИМАНИЕ: смещение в файле не меняется, перед чтением позиция должна находиться либо в начале файла, либо после позиции сообщения, предшествующего первому считываемому)
{
    struct s_message message;
    int i;
	for (i = 0; read_single_message(room, &message) != 1; i++)
	{	  

	}	
    return i;
}

int read_single_message(int room, struct s_message* msg)
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

int write_message(int room, char* datetime, char* nickname, char* msg, int number)	//Запись сообщения в историю
{
	/*	Хранение истории происходит в следующем формате:
	<1> - номер сообщения
	<01/02/2003> - дата и время отправления
	<Alice> - никнейм отправителя
	<Hello, world> - сообщение
	<Позиция> - указывает на предыдущую позицию (т.е. перед номером текущего сообщения)
	*/
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

    /*
    int server_socket = socket (AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    int port;
    while (1)
    {
        
        char ipstr[16];
        if (!fgets(ipstr, 16, stdin));
        {
            clear();
            printf ("Ошибка ввода!\n\n");
            continue;
        }
        clear();
        if (inet_aton(ipstr, &address.sin_addr)==0)
        {
            clear();
            printf ("Неправильный формат IP адреса!\n\n");
            continue;
        }
        printf("\tВведите порт, по которому будет производиться подключение\n>");
        if (scanf("%i", &port) != 1);
        {
            clear();
            printf ("Ошибка ввода!\n\n");
            continue;
        }
        clear();
        if (port < 0 || port > 65535)
        {
            clear();
            printf ("Неправильный формат порта!\n\n");
            continue;
        }
          
        else break; 
    }*/