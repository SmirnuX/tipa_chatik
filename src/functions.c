#include "main.h"

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

int is_correct_name(char* str)  //Проверяет название комнаты на корректность (не должно начинаться с точки и соблюдать все правила именования файлов в ос)
{
    char restricted[] = "/\\?<>*|"; //Запрещенные символы в названии
    if (str[0] == '.')
        return 0;
    for (int i = 0; i < strlen(str); i++)
        for (int j = 0; j < strlen(restricted); j++)
            if (str[i] == restricted[j])
                return 0;
    return 1;
}

char* tipa_gets(char* dest, int max, int fd)  //Низкоуровневый аналог fgets
{
    int read_count;
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
        read(fd, &tmp, 1);
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