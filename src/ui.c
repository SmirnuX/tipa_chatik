#include "main.h"
//Содержит в себе функции для отображения консольного интерфейса

char reconnect_animation[RECONNECT_FRAMES][RECONNECT_FRAMELEN] = {
    "(╮°-°)╮   ┳━┳   ",
    "(╯°□°)╯︵ ┻━┻",
};


int client_ui_download_files(struct s_connection* connection, int* selected_room)   //Загрузка файлов с сервера
{
    int page = 1;
    while (1)
    {
        //Получение списка файлов с сервера
        if (get_files_client(connection, *selected_room, page) == ECONNREFUSED)
        {
            clear()
            printf ("Произошли изменения на сервере. Переподключение...\n\n");
            *selected_room = -1;
            return -1;
        }
        printf( WHITE BRIGHT "\n============================================================\n"
                DEFAULT "\t\tСписок файлов %s\n", rooms[*selected_room]->name);            
        //Вывод списка
        for (int i = (page-1)*FILESPERPAGE; i < page*FILESPERPAGE; i++)
        {	
            if (i == MAXFILES || i == rooms[*selected_room]->file_count)
                break;
            printf("\t%i. %s\n", i+1, rooms[*selected_room]->file_names[i]);
        }
        printf( BRIGHT  "\t0. Отменить выбор файла\n");
        if (page == 1)
            printf(DIM);
        printf("\t<A"DEFAULT"\t Страница %i из %i \t", page, rooms[*selected_room]->file_count/FILESPERPAGE + 1);              
        if (page == rooms[*selected_room]->file_count/FILESPERPAGE + 1)
            printf(DIM);
        printf("D>\n"   DEFAULT BRIGHT  "============================================================\n"
                        DEFAULT         "Введите номер выбранного файла или A/D для перехода на другую страницу: ");
        char buf[MAXBUFFER];
        tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
        int choice = atoi(buf);
        switch(buf[0])
        {
            case 'A':   //Предыдущая страница
            case 'a':
            if (page != 0)
                page--;
            break;
            case 'D':   //Следующая страница
            case 'd':
            if (page < (rooms[*selected_room]->file_count/FILESPERPAGE + 1))
                page++;
            break;
            case '0':   //Выход из меню
            return 0;
            break;
            default:    //Загрузка файлов
            if (choice <= 0 || choice > rooms[*selected_room]->file_count)
            {
                clear()
                printf(BRIGHT RED"Неправильно набран номер файла.\n\n"DEFAULT WHITE);
                continue;
            }
            else if (download_file_client(connection, *selected_room, choice-1) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                *selected_room = -1;
                return -1;
            }       
        }
    }
}

int client_ui_select_room(char* server_name)
{
    printf( WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", server_name);            
    for (int i = 0; i < room_count; i++)
    {
        rooms[i]->fd = open(rooms[i]->name, O_CREAT | O_RDWR, PERMISSION);  //Создание файлов истории
        lseek(rooms[i]->fd, 0, SEEK_SET);
        rooms[i]->msg_count = count_messages(rooms[i]->fd);
        printf("\t%i. %s (%i сохр. сообщ-й)\n", i+1, rooms[i]->name, rooms[i]->msg_count);
    }
    printf( BRIGHT  "\t0. Закрыть программу\n"
                    "============================================================\n"
            DEFAULT "Введите номер выбранной комнаты: \n");
    char buf[MAXBUFFER];
    tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
    int choice = atoi(buf);
    if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
    {
        ui_show_error("Неправильно набран номер комнаты.", 0);
        return -1;
    }
    else
        return choice;
}

int client_ui_select_action(int selected_room)
{
    printf("\tСообщения %s\n\n", rooms[selected_room]->name);
    //Отображение истории сообщений
    lseek(rooms[selected_room]->fd, 0, SEEK_SET);
    clear()
    read_messages(rooms[selected_room]->fd);
    //===Меню выбора действия===
    printf( WHITE BRIGHT "============================================================\n"
    DEFAULT "\tДействия в \"%s\"\n"
            "\t1. Написать сообщение\n"
            "\t2. Обновить\n"
            "\t3. Загрузить файл на сервер\n"
            "\t4. Скачать файл с сервера\n"
    BRIGHT  "\t0. Выйти из комнаты\n"
            "============================================================\n"
    DEFAULT "Введите номер выбранного действия: ", rooms[selected_room]->name);
    char choice = getchar();
    while (getchar() !='\n');   //Очистка буфера
    if (choice >= '0' && choice <='4')
        return choice - '0';
    else
        return -1;
}

int client_ui_send_message(int selected_room, struct s_connection* connection)
{
    if (get_new_messages_client(connection, selected_room, rooms[selected_room]->msg_count) == ECONNREFUSED)
    {
        printf ("Произошли изменения на сервере. Переподключение...\n\n");
        return -1;
    }
    char buf[MAXBUFFER];
    lseek(rooms[selected_room]->fd, 0, SEEK_SET);
    read_messages(rooms[selected_room]->fd);
    printf( 
    WHITE BRIGHT"=======================================================================\n"
    DEFAULT     "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
    DIM         "\tДля отмены отправки сообщения сотрите все символы и нажмите ENTER.\n"
    DEFAULT BRIGHT"=======================================================================\n"
    DEFAULT);
    if (tipa_gets(buf, MAXBUFFER, STDIN_FILENO) != NULL)
        if (buf[0] != '\n' && buf[0] != '\0')
        {
            if (send_message_client(connection, selected_room, nickname, buf) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                return -1;
            }
        }
    return 0;
}

int client_ui_send_file(int selected_room, struct s_connection* connection)
{
    char buf[MAXBUFFER];
    if (chdir("..") != 0)
        ui_show_error("Ошибка перемещения по директориям.", ENOENT);
    if (getcwd(buf, MAXBUFFER) == NULL)
        ui_show_error("Ошибка перемещения по директориям.", errno);
    printf( 
    WHITE BRIGHT"=======================================================================\n"
    DEFAULT     "\tВведите путь к загружаемому файлу и ENTER для отправки файла\n"
    DIM         "\tДля отмены отправки файла сотрите все символы и нажмите ENTER.\n"
                "\tФайлы должны находиться в директории с чатом: %s\n"
    DEFAULT BRIGHT"=======================================================================\n"
    DEFAULT, buf);
    if (tipa_gets(buf, MAXBUFFER, STDIN_FILENO) != NULL)
        if (buf[0] != '\n' && buf[0] != '\0')
        {
            remove_new_line(buf);
            int file = open(buf, O_RDONLY);
            if (file < 0)
            {
                clear()
                ui_show_error("Ошибка открытия файла.", 1);
                errno = 0;
                if (chdir("client_history") != 0)
                    ui_show_error("Ошибка перемещения по директориям.", ENOENT);
                return -1;
            }
            else if (send_file_client(connection, selected_room, file, buf) == ECONNREFUSED)
            {
                clear()
                printf ("Произошли изменения на сервере. Переподключение...\n\n");
                if (chdir("client_history") != 0)
                    ui_show_error("Ошибка перемещения по директориям.", ENOENT);
                return -1;
            }
        }
    if (chdir("client_history") != 0)
        ui_show_error("Ошибка перемещения по директориям.", ENOENT);
    return 0;
}

int ui_show_error(char* error, int show_errno)  //Вывод окна ошибки с текстом error. Если show_errno не равен нулю, также выведется сообщение об ошибке согласно errno.h
{
    printf( DEFAULT
    RED BRIGHT  "\n=======================================================================\n"
                "\t%s\n", error);
    if (show_errno != 0)
        perror("\t");
    printf(
    RED          "\tНажмите любую кнопку чтобы продолжить.\n"
    BRIGHT      "=======================================================================\n"
    WHITE DEFAULT);
    while (getchar() != '\n');
    clear()
    return show_errno;
}

int server_ui_main_menu()
{
    printf(	DEFAULT BRIGHT 	
                    "============================================================\n"	
            DEFAULT	"Доступны следующие функции:\n"
                    "1. Создать новую комнату\n"
                    "2. Написать сообщение в одну из комнат\n"
                    "3. Просмотреть список файлов комнаты\n"
                    "4. Удалить комнату\n"
            BRIGHT	"0. Закрыть сервер\n"
                    "============================================================\n" DEFAULT DIM);
    erase_lines(SERVER_MENU_LINES);
    return 0;
}

int server_ui_create_room()
{
    char buf[MAXBUFFER];
    printf( DEFAULT
            WHITE BRIGHT "============================================================\n"
            DEFAULT "\tСоздание новой комнаты\n"
            DIM		"Введите название комнаты и нажмите ENTER.\n"
                    "Для отмены создания комнаты сотрите все символы и нажмите ENTER\n"
    DEFAULT BRIGHT  "============================================================\n" DEFAULT);
    if (tipa_gets(buf, MAXBUFFER, STDIN_FILENO) != NULL)
    {
        erase_lines(SERVER_NEW_ROOM_LINES + 1);
        if (buf[0] != '\n' && buf[0] != '\0')
        {
            remove_new_line(buf);
            if (!is_correct_name(buf))
                ui_show_error("Название комнаты не должно начинаться с точки или содержать символы  / ? < > * \" |", 0);
            else if (room_count == MAXROOMS)
                ui_show_error("Достигнуто максимальное количество комнат.", 0);
            else
            {
                int i;
                for (i = 0; i < room_count; i++)
                {
                    if (strcmp(buf, rooms[i]->name) == 0)
                        break;
                }
                if (i < room_count)	//Если найдено совпадение
                    ui_show_error("Комната с таким названием уже существует.", 0);
                else
                {
                    if (rooms[room_count+1] == NULL)	//Инициализация комнаты
                    {
                        rooms[room_count+1] = malloc(sizeof(struct s_room));
                        rooms[room_count+1]->name = malloc(sizeof(char) * MAXNICKLEN);
                        rooms[room_count+1]->msg_count = 0;
                        rooms[room_count+1]->file_count = 0;
                        for (int i=0; i < MAXFILES; i++)
						    rooms[room_count+1]->file_names[i] = NULL;
                    }
                    strncpy(rooms[room_count+1]->name, buf, MAXNICKLEN);
                    rooms[room_count+1]->fd = open(buf, O_CREAT | O_RDWR, PERMISSION);
                    if (rooms[room_count+1]->fd < 0)
                    {
                        ui_show_error("Ошибка создания комнаты.", 1);
                        free(rooms[room_count+1]->name);
                        free(rooms[room_count+1]);
                    }
                    else
                    {
                        refresh_files_room(room_count+1);
                        room_count++;
                        server_time = time(NULL);	//Обновление "версии" сервера
                        printf(	"\n Добавлена комната %s.\n"
                                "Время обновления: %li\n", rooms[room_count]->name, server_time);
                        rooms_sort();
                    }
                }
            }
        }
    }
    return 0;
}

int server_ui_send_message()
{
    char buf[MAXBUFFER];
    printf( DEFAULT
            WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", nickname);            
    for (int i = 0; i < room_count; i++)
        printf("\t%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i]->name, lseek(rooms[i]->fd, 0, SEEK_END), rooms[i]->msg_count);
    printf( BRIGHT  "\t0. Отменить написание сообщения\n"
            RED  	"На время написания сообщения работа сервера приостановлена\n"
            WHITE	"============================================================\n"
            DEFAULT "Введите номер выбранной комнаты: ");
    tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
    erase_lines(SERVER_CHOOSE_ROOM_LINES + 1 + room_count);
    int choice = atoi(buf);
    if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
        ui_show_error("Неправильно набран номер комнаты.", 0);
    else if (choice != 0)
    {
        choice--;
        printf( DEFAULT
                WHITE BRIGHT"=======================================================================\n"
                DEFAULT     "\tВведите сообщение и нажмите ENTER для отправки сообщения\n"
                DIM         "\tДля отмены отправки сообщения сотрите все символы и нажмите ENTER.\n"
                DEFAULT BRIGHT"=======================================================================\n"
                DEFAULT);  
        if (tipa_gets(buf, MAXBUFFER, STDIN_FILENO) != NULL)
        {
            erase_lines(SERVER_WRITE_MESSAGE_LINES + 1);
            if (buf[0] != '\n' && buf[0] != '\0')
            {
                char s_time[MAXBUFFER];
                time_t timer = time(NULL);
                struct tm loc_time;
                localtime_r(&timer, &loc_time);
                strftime(s_time, MAXBUFFER, "%H:%M %d.%m.%Y ", &loc_time);
                buf[MAXBUFFER - 1] = '\0';
                write_message(rooms[choice]->fd, s_time, nickname, buf, ++(rooms[choice]->msg_count));
                printf(	"\n Сообщение отправлено.\n");
            }
        }
    }
    return 0;
}

int server_ui_delete_room()
{
    char buf[MAXBUFFER];
    printf( WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", nickname);            
    for (int i = 0; i < room_count; i++)
        printf("\t%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i]->name, lseek(rooms[i]->fd, 0, SEEK_END), rooms[i]->msg_count);
    printf( BRIGHT  "\t0. Отменить удаление комнаты\n"
            RED  	"На время удаления комнаты работа сервера приостановлена\n"
            WHITE	"============================================================\n"
            DEFAULT "Введите номер удаляемой комнаты: ");
    tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
    erase_lines(SERVER_CHOOSE_ROOM_LINES + 1 + room_count);
    int choice = atoi(buf);
    if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
        ui_show_error("Неправильно набран номер комнаты.", 0);
    else if (choice != 0)
    {
        choice--;
        close(rooms[choice]->fd);
        if (remove(rooms[choice]->name) == -1)
            ui_show_error("Ошибка удаления комнаты.", 1);
        else
        {
            server_time = time(NULL);	//Обновление "версии" сервера
            printf(	"\n Удалена комната %s.\n"
                    "Время обновления: %li\n", rooms[choice]->name, server_time);
            for (int j=0; j < MAXROOMS; j++)
				if (rooms[choice]->file_names[j] == NULL)
					free(rooms[choice]->file_names[j]);
            free(rooms[choice]->name);
            free(rooms[choice]);
            rooms[choice] = NULL;
            rooms_sort();
            room_count--;
        }
    }
    return 0;
}

int server_ui_view_files()
{
    char buf[MAXBUFFER];
    printf( WHITE BRIGHT "\n============================================================\n"
            DEFAULT "\t\tСписок комнат сервера %s\n", nickname);            
    for (int i = 0; i < room_count; i++)
        printf("\t%i.%s (%li байт, %i сообщений)\n", i+1, rooms[i]->name, lseek(rooms[i]->fd, 0, SEEK_END), rooms[i]->msg_count);
    printf( BRIGHT  "\t0. Выйти в меню\n"
            RED  	"На время просмотра файлов работа сервера приостановлена\n"
            WHITE	"============================================================\n"
            DEFAULT "Введите номер комнаты: ");
    tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
    erase_lines(SERVER_CHOOSE_ROOM_LINES + 1 + room_count);
    int choice = atoi(buf);
    if (choice < 0 || choice > room_count || (choice == 0 && buf[0] != '0'))
        ui_show_error("Неправильно набран номер комнаты.", 0);
    else
    {
        choice--;
        int page = 1;
        while(1)
        {
            printf( WHITE BRIGHT "\n============================================================\n"
                    DEFAULT "\t\tСписок файлов %s\n", rooms[choice]->name);            
            //Вывод списка
            for (int i = (page-1)*FILESPERPAGE; i < page*FILESPERPAGE; i++)
            {	
                if (i == MAXFILES || i == rooms[choice]->file_count)
                    break;
                printf("\t%i. %s\n", i+1, rooms[choice]->file_names[i]);
            }
            printf( BRIGHT  "\t0. Выйти\n");
            if (page == 1)
                printf(DIM);
            printf("\t<A"DEFAULT"\t Страница %i из %i \t", page, rooms[choice]->file_count/FILESPERPAGE + 1);              
            if (page == rooms[choice]->file_count/FILESPERPAGE + 1)
                printf(DIM);
            printf("D>\n"   BRIGHT "============================================================\n"
                    DEFAULT "Введите 0 для выхода или A/D для перехода на другую страницу: ");
            char buf[MAXBUFFER];
            tipa_gets(buf, MAXBUFFER, STDIN_FILENO);
            int lines = FILESPERPAGE - (page*FILESPERPAGE - MAXFILES);
            erase_lines(SERVER_VIEW_FILES_LINES + 1 + lines);
            switch(buf[0])
            {
                case 'A':   //Предыдущая страница
                case 'a':
                if (page != 1)
                    page--;
                break;
                case 'D':   //Следующая страница
                case 'd':
                if (page < (rooms[choice]->file_count/FILESPERPAGE + 1))
                    page++;
                break;
                case '0':   //Выход из меню
                return 0;
                break;    
            } 
        }
    }
    return 0;
}

int client_ui_reconnect(int n)
{
    printf( DEFAULT BRIGHT    "\n============================================================\n"
                            "\t\tПереподключение\n");
    if (n == 0)
        printf("\n\n============================================================\n");
    else
    {
        printf( "\tПопытка %i \t%s\n\tДля завершения работы программы нажмите ENTER.\n"
                "============================================================\n",
                n, reconnect_animation[n % RECONNECT_FRAMES]);
        sleep(1);
        int available;
        ioctl(STDIN_FILENO, FIONREAD, &available);	//Считываем количество бит, которые можно прочитать
        if (available != 0 || n == MAXRECONNECTRIES)
        {
            erase_lines(CLIENT_RECONNECT_LINES);
            client_safe_exit();
        }
    }
    erase_lines(CLIENT_RECONNECT_LINES);
    return 0;
}