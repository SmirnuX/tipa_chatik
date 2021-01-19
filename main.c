#include "main.h"

int main(int argc, char* argv[])
{
    

    
    char* config_file;
    short is_server; 
    char ipaddr[MAXIPLEN+1];
    int port;
    char nickname[MAXNICKLEN+1];

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
            respond = bind(sock, (struct sockaddr *) &address, sizeof(address));

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
            client(sock, nickname);
        else
            server(sock, nickname);
    
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