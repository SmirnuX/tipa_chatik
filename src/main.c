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
            if (write(config, ipaddr, strlen(ipaddr)) <= 0 ||
                write(config, "\n", sizeof(char)) <= 0 ||
                write(config, port_str, strlen(port_str)) <= 0 ||
                write(config, "\n", sizeof(char)) <= 0 ||
                write(config, nickname, strlen(nickname)) <= 0)
            {
                ui_show_error("Ошибка записи в файл конфигурации.", 1);
                return 1;
            }
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
                ui_show_error("Ошибка файла конфигурации.", 1);
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
            ui_show_error("Ошибка файла конфигурации: неправильный формат.", 0);
            return -1;
        }
    }
    return i;
}

int goto_message(int room, int count)	//Перемещение позиции в файле к count сообщению с конца
{
	char buf[MAXBUFFER];
    long pos = 0;
	lseek(room, -SIZEOF_MAXLENGTH, SEEK_END); //Чтение позиции последнего сообщения
	for (int i = 0; i < count; i++)
	{
		if (read(room, buf, SIZEOF_MAXLENGTH) != SIZEOF_MAXLENGTH)
            return -1;
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
    return 0;
}

int read_messages(int room)	//Вывод всех сообщений из файла, соответствующего комнате под номером room, начиная с установленной ранее позиции в файле
{
    struct s_message message;
    int i;
	for (i = 0; read_single_message(room, &message) != 1; i++)
	{	  
        printf(DEFAULT BLUE"%s\n", message.datetime);
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
    if(
        write(room, buf, strlen(buf)+1) != strlen(buf)+1 ||
        write(room, datetime, strlen(datetime)+1) != strlen(datetime)+1 ||
        write(room, nickname, strlen(nickname)+1) != strlen(nickname)+1 ||
        write(room, msg, strlen(msg)+1) != strlen(msg)+1
    )
    {
        return -1;
    }
	snprintf(buf, MAXBUFFER, SIZEOF_MAXLENGTH_FORMAT, pos - SIZEOF_MAXLENGTH);
    if (write(room, buf, SIZEOF_MAXLENGTH) != SIZEOF_MAXLENGTH)
        return -1;
    return 0;
}

void remove_new_line(char* str) //Убирает последний символ новой строки (если есть)
{
    if (str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';
}

int is_correct_name(char* str)  //Проверяет название комнаты на корректность (не должно начинаться с точки и соблюдать все правила именования файлов в ос)
{
    char restricted[] = "/\\?<>*|"; //Запрещенные символы в названии
    if (str[0] == '.')
        return 0;
    for (uint i = 0; i < strlen(str); i++)
        for (uint j = 0; j < strlen(restricted); j++)
            if (str[i] == restricted[j])
                return 0;
    return 1;
}

char* tipa_gets(char* dest, int max, int fd)  //Низкоуровневый аналог fgets
{
    int read_count = -1;
    char tmp;
    for(int i = 0; i < max; i++)
    {
        read_count = read(fd, &tmp, 1);
        if (read_count < 0)
        {
            perror("tipa_gets error: ");
            return NULL;
        }
        if (read_count == 0 || tmp == '\n')    //EOF
        {
            dest[i] = '\0';
            return dest;
        }
        dest[i] = tmp;
    }
    dest[max-1] = '\0';
    //Считывание остатка строки
    do
        read_count = read(fd, &tmp, 1);
    while (read_count == 1 && tmp != '\n');
    return dest;
}

int mkchdir(char* path)
{
    if (chdir(path) != 0)
    {
        mkdir(path, FOLDERPERMISSION);
        return chdir(path);
    }
    else
        return 0;
}