//
//  Created by Boris Stasenko on 07.12.2019
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"
#pragma clang diagnostic ignored "-Wunknown-pragmas"

#include "common.h"
#include "server.h"

//  имя программы
char *program_name;

//  глобальные значения для работы приложения
struct globalArgs_t {
    #ifdef SERVER
    int daemon;
    double version;
    int imitate;
    char *logfile;
    #else
    char *message;
    #endif
    char *ip_addr;
    int ip_port;
} globalArgs;

//  запуск программы
int main(int argc, char **argv) {
    parse(argc, argv);

    #ifdef SERVER
    if (globalArgs.daemon) run_as_daemon();
    logger = fopen(globalArgs.logfile, "a");

    if (!logger) display_usage("<MAIN> Opening log");
    p_log(DOUBLE_LOG, "Server is running\n");

    server(globalArgs.ip_addr, globalArgs.ip_port, globalArgs.imitate);
    p_log(DOUBLE_LOG, "Server shutdown\n");
    fclose(logger);
    #else
    client(globalArgs.ip_addr, globalArgs.ip_port, globalArgs.message);
    #endif
    return 0;
}

//  справка по программе
void display_usage(char *error) {
    if (error) perror(error);

    char *format = "Usage: %s [options]\n"
                   "\tOptions:\n"
                   "\t\t-h show usage\n"
                   #ifdef SERVER
                   "\t\t-v show version\n"
                   "\t\t-d run as daemon\n"
                   "\t\t-w N emulate work: sleep N seconds (default 0, option may be passed through environment variable L2WAIT)\n"
                   "\t\t-l LOGFILE specify logfile (default '/tmp/lab2.log', option may be passed through environment variable L2LOGFILE)\n"
                   #else
                   "\t\t-m MESSAGE message to send to server"
                   #endif
                   "\t\t-a IP-ADDRESS determine ip address to listen (option may be passed through environment variable L2ADDR)\n"
                   "\t\t-p IP-PORT determine port to listen (option may be passed through environment variable L2PORT)\n";

    printf(format, program_name);
    exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

//  установка глабальных значений
void parse(int argc, char **argv) {
    char *tmp;

    #ifdef SERVER
    globalArgs.daemon = 0;
    globalArgs.version = 0;
    globalArgs.imitate = 0;
    globalArgs.logfile = "/tmp/lab2.log";

    tmp = getenv("L2WAIT");
    if (tmp) sscanf(tmp, "%u", &globalArgs.imitate);
    tmp = getenv("L2LOGFILE");
    if (tmp) globalArgs.logfile = strdup(tmp);
    #else
    globalArgs.message = NULL;
    #endif

    globalArgs.ip_addr = NULL;
    globalArgs.ip_port = 0;
    program_name = argv[0];

    tmp = getenv("L2ADDR");
    if (tmp) globalArgs.ip_addr = strdup(tmp);
    tmp = getenv("L2PORT");
    if (tmp) sscanf(tmp, "%u", &globalArgs.ip_port);

    //  устанавливаем значения опций
    #ifdef SERVER
    const char *optString = "w:dl:a:p:vh?";
    #else
    const char *optString = "m:a:p:h?";
    #endif

    int opt = 0;
    do {
        switch (opt) {
            #ifdef SERVER
            case 'v':
                globalArgs.version = 1.0;
                printf("%s version %.2f\n", program_name, globalArgs.version);
                exit(EXIT_SUCCESS);
            case 'd':
                globalArgs.daemon = 1;
                break;
            case 'w':
                sscanf(optarg, "%u", &globalArgs.imitate);
                break;
            case 'l':
                globalArgs.logfile = optarg;
                break;
            #else
            case 'm':
                globalArgs.message = optarg;
                break;
            #endif
            case 'a':
                globalArgs.ip_addr = optarg;
                break;
            case 'p':
                sscanf(optarg, "%u", &globalArgs.ip_port);
                break;
            case 'h':
                display_usage(0);
                break;
            case '?':
                break;
        }
        opt = getopt(argc, argv, optString);
    } while (opt != -1);

    if (!globalArgs.ip_addr || !globalArgs.ip_port || !inet_aton(globalArgs.ip_addr, 0))
        display_usage("Address and port is required options and must be valid");

}

#pragma clang diagnostic pop