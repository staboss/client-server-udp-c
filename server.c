//
//  Created by Boris Stasenko on 07.12.2019
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-narrowing-conversions"
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma clang diagnostic ignored "-Wpointer-sign"
#pragma clang diagnostic ignored "-Wunknown-pragmas"

#include "server.h"
#include "common.h"
#include "sys/stat.h"
#include <pthread.h>
#include <fcntl.h>

//  тип подключения клиент / сервер
enum CONNECTION_TYPES {
    Server, Client
};

//  для отправки и получения данных
typedef struct arg_struct {
    char *data;
    int data_len;
    struct sockaddr_in *address;
    socklen_t len;
    int imitate;
} arg_struct;

//  количество успешно / неуспешно обработанных запросов
int succeed, errors;

//  создание сокета
int create_socket(char *ip, int port, struct sockaddr_in *addr, enum CONNECTION_TYPES type) {

    int sock = 0;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        p_log(DOUBLE_LOG, 0);
        exit(EXIT_FAILURE);
    }

    addr->sin_family = AF_INET;                 //  IP4 Internetwork: UDP, TCP
    addr->sin_addr.s_addr = inet_addr(ip);      //  Устанавливаем порт
    addr->sin_port = htons(port);               //  Устанавливаем хост

    int opt_val = 1;
    //  SOL_SOCKET - манипуляции флагами на уровне сокета
    //  SO_REUSEADDR - опция разрешает (1) / запрещает (0) использование локальных адресов
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));

    if (type == Server) {
        //  установка сокета для неблокируемого ввода-вывода
        fcntl(sock, F_SETFL, O_NONBLOCK);

        //  привязка сокета к определенному порту и адресу
        if (bind(sock, (const struct sockaddr *) addr, sizeof(*addr)) < 0) {
            p_log(DOUBLE_LOG, 0);
            exit(EXIT_FAILURE);
        }
    }

    return sock;
}

//  обработчики сигналов
void term_handler();

void quit_handler();

void usr1_handler();

void set_sig_handlers() {

    struct sigaction term_act, quit_act, usr1_act;

    memset(&term_act, 0, sizeof(term_act));
    term_act.sa_handler = term_handler;

    memset(&quit_act, 0, sizeof(quit_act));
    quit_act.sa_handler = quit_handler;

    memset(&usr1_act, 0, sizeof(usr1_act));
    usr1_act.sa_handler = usr1_handler;

    sigaction(SIGTERM, &term_act, 0);           //  завершить программу
    sigaction(SIGQUIT, &quit_act, 0);           //  завершение с дампом памяти
    sigaction(SIGUSR1, &usr1_act, 0);           //  сигналы, определяемые пользователем
    sigaction(SIGINT, &quit_act, 0);            //  пользователь нажал CTRL-C
}

//  получить сообщение
int receive_message(int sock, void **arg) {

    char tmp_buf[MAX_DATA_UDP + 1] = {0};
    struct sockaddr_in tmp_addr;

    int tmp_len = sizeof(tmp_addr);
    int tmp_data_len;

    arg_struct *args;

    //  получение сообщений из сокета
    //  MSG_WAITALL - блокирование операции до полной обработки запроса
    if (*arg && (args = *arg)) {
        tmp_data_len = recvfrom(sock, tmp_buf, MAX_DATA_UDP, MSG_WAITALL,
                                (struct sockaddr *) args->address, &args->len);
    } else {
        tmp_data_len = recvfrom(sock, tmp_buf, MAX_DATA_UDP, MSG_WAITALL,
                                (struct sockaddr *) &tmp_addr, &tmp_len);
    }

    if (tmp_data_len > 0) {

        if (*arg == NULL) {
            *arg = calloc(1, sizeof(arg_struct));
            args = *arg;
            args->len = tmp_len;
            args->address = calloc(1, sizeof(struct sockaddr_in));
            memcpy(args->address, &tmp_addr, tmp_len);
        }

        args->data = calloc(tmp_data_len + 1, sizeof(char));
        memcpy(args->data, tmp_buf, tmp_data_len);
        args->data_len = tmp_data_len;

        if (args->address) {
            char host[20];
            inet_ntop(AF_INET, &args->address->sin_addr, host, sizeof(host));
            p_log(DOUBLE_LOG, "Connection from %s:%d\n", host, ntohs(args->address->sin_port));
            p_log(DOUBLE_LOG, "RECV from %s:%d\n", host, ntohs(args->address->sin_port));
        }

        return args->data_len;

    } else return -1;
}

//  отправить сообщение
int send_message(int sock, void **arg) {
    arg_struct *args = *arg;

    if (args->data_len) {
        char host[20];
        inet_ntop(AF_INET, &args->address->sin_addr, host, sizeof(host));
        p_log(DOUBLE_LOG, "SEND to %s:%d\n", host, ntohs(args->address->sin_port));
        return sendto(sock, args->data, strlen(args->data), 0, (const struct sockaddr *) args->address, args->len);
    }

    return 1;
}

//  закрываем соединение и освобождаем память
int close_connection(int sock, void **arg) {
    arg_struct *args = *arg;

    char host[20];
    inet_ntop(AF_INET, &args->address->sin_addr, host, sizeof(host));
    p_log(DOUBLE_LOG, "Close connection with %s:%d\n", host, ntohs(args->address->sin_port));

    free(args->data);
    free(args->address);
    free(args);

    return 0;
}

int obj_count = 0;      //  счетчик для массива потоков
void **array = NULL;    //  массив потоков

//  монитор / mutex
pthread_mutex_t stats = PTHREAD_MUTEX_INITIALIZER;

//  добавляет поток в массив, чтобы потом иметь
//  возможность дождаться завершения потоков при сигнале
void add_to_wait(void *obj) {
    obj_count++;
    array = realloc(array, obj_count * sizeof(pthread_t *));
    ((pthread_t **) array)[obj_count - 1] = obj;
}

//  обновление статистики обслуживания запросов
void update_stats(int stat) {
    pthread_mutex_lock(&stats);
    stat ? succeed++ : errors++;
    pthread_mutex_unlock(&stats);
}

//  строка в кодировке base64
int function_base64(char *request, char *response) {
    char *encode = b64_encode((const unsigned char *) request, strlen(request));
    strncpy(response, encode, strlen(encode));
    free(encode);
    return 1;
}

//  для многопоточности
typedef struct args_multi_thread {
    arg_struct *args;
    int c_socket;
} args_multi_thread;

//  обработка потока
void *thread_program(void *arg) {
    args_multi_thread *args = arg;
    int sock = args->c_socket;

    char *response = calloc(MAX_DATA_UDP + 1, sizeof(char));
    int success = function_base64(args->args->data, response);
    free(args->args->data);

    args->args->data = response;
    args->args->data_len = strlen(response);

    if (args->args->imitate) sleep(args->args->imitate);
    if (send_message(sock, (void **) &args->args) == -1) p_log(DOUBLE_LOG, 0);

    update_stats(success);
    close_connection(sock, (void **) &args->args);
    free(args);

    return NULL;
}

//  запуск многопоточности
void *create_multithreading(int c_sock, void *request) {
    pthread_t *tid = calloc(1, sizeof(pthread_t));
    args_multi_thread *args = calloc(1, sizeof(args_multi_thread));

    args->c_socket = c_sock;
    args->args = request;

    pthread_create(tid, NULL, thread_program, args);
    return tid;
}

int kill_all = 0;       //  для остановки цикла
time_t start_time;      //  время начала работы сервера

//  возвращает переданное значение сокета
int connection(int sock) {
    return sock;
}

//  программа сервера:
//  получить сообщение -> запустить нить -> добавить нить в массив нитей
void server(char *ip, int port, int imitate) {
    start_time = time(NULL);
    set_sig_handlers();

    struct sockaddr_in server_addr;
    int sock = create_socket(ip, port, &server_addr, Server);

    while (!kill_all) {
        int c_sock;
        arg_struct *arg = NULL;

        if ((c_sock = connection(sock)) > 0 && receive_message(c_sock, (void **) &arg) > 0) {
            arg->imitate = imitate;
            void *thread_id = create_multithreading(c_sock, arg);
            add_to_wait(thread_id);
        }
    }
}

//  программа клиента:
//  отправить сообщение -> получить сообщение
void client(char *ip, int port, char *message) {
    struct sockaddr_in *server_addr = calloc(1, sizeof(struct sockaddr_in));

    int sock = create_socket(ip, port, server_addr, Client);
    int c_sock;

    arg_struct *arg = calloc(1, sizeof(arg_struct));

    if ((c_sock = connection(sock)) > 0) {
        arg->address = server_addr;
        arg->data = message;
        arg->data_len = strlen(message);
        arg->len = sizeof(*arg->address);

        send_message(c_sock, (void **) &arg);
        receive_message(c_sock, (void **) &arg);

        printf("%s\n", arg->data);
        close_connection(c_sock, (void **) &arg);
    }
}

//  запуск в режиме демона
void run_as_daemon(void) {

    //  закрываем дескрипторы ввода/вывода ошибок
    close(fileno(stdin));
    close(fileno(stdout));
    close(fileno(stderr));

    int fd[2];
    pipe(fd);

    char pid[sizeof(unsigned int)] = {0};

    //  создаем дочерний процесс
    pid_t pid_1 = fork();

    if (pid_1 < 0) {
        perror("Daemon fork 1");
        exit(EXIT_FAILURE);
    } else if (pid_1 == 0) {

        //  отсоединяем от терминала и создаем независимый сеанс
        setsid();

        //  в дочернем fork() снова вызываем fork(), чтобы
        //  демон никогда не смог повторно получить терминал
        pid_t pid_2 = fork();

        if (pid_2 < 0) {
            perror("Daemon fork 2");
            exit(EXIT_FAILURE);
        } else if (pid_2 == 0) {

            //  в процессе демона подключаем /dev/null к стандартному вводу, выводу и ошибкам
            FILE *f_tmp = fopen("/dev/null", "rb");
            dup(fileno(f_tmp));
            close(fileno(f_tmp));

            f_tmp = fopen("/dev/null", "wb");
            dup(fileno(f_tmp));
            dup(fileno(f_tmp));
            close(fileno(f_tmp));

            //  разрешаем выставлять все биты прав на создаваемые файлы,
            //  иначе могут возникнуть проблемы с правами доступа
            umask(0);

            //  изменяем текущий каталог на корневой каталог (/), для того,
            //  чтобы избежать как демон невольно заблокирует точки монтирования
            chdir("/");

            //  записываем PID демона, чтобы демон не мог быть запущен более одного раза
            (*((unsigned int *) pid)) = getpid();
            write(fd[1], pid, sizeof(unsigned int));

            close(fd[0]);
            close(fd[1]);
        } else {
            exit(EXIT_SUCCESS);
        }
    } else {
        read(fd[0], pid, sizeof(unsigned int));
        exit((*((unsigned int *) pid)) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
}

//  секунды в чч:мм:сс
void sec_to_time(int sec, char *buf) {
    int h, m, s;
    h = (sec / 3600);
    m = (sec - (3600 * h)) / 60;
    s = (sec - (3600 * h) - (m * 60));
    sprintf(buf, "%04d:%02d:%02d", h, m, s);
}

//  прекратить создавать новые нити и ждать завершение запущенных
void quit_handler() {
    kill_all = 1;
    for (int i = 0; i < obj_count; i++) {
        if (((pthread_t **) array)[i]) {
            pthread_join(*(((pthread_t **) array)[i]), NULL);
            free(((pthread_t **) array)[i]);
            ((pthread_t **) array)[i] = NULL;
        }
    }
    p_log(DOUBLE_LOG, "Threads were terminated\n");
}

//  выйти и завершить запущенные нити
void term_handler() {
    p_log(DOUBLE_LOG, "SIGTERM\n");
    quit_handler();
    free(array);
    exit(EXIT_SUCCESS);
}

//  отобразить статистику
void usr1_handler() {
    time_t end = time(NULL);
    double time_spent = difftime(end, start_time);
    char diff[12];
    sec_to_time(time_spent, diff);
    p_log(DOUBLE_LOG, "Server was started ~%s ago\n", diff);
    pthread_mutex_lock(&stats);
    p_log(DOUBLE_LOG, "STATISTICS: Errors: %d | Succeed: %d\n", errors, succeed);
    pthread_mutex_unlock(&stats);
}

#pragma clang diagnostic pop