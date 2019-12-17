//
//  Created by Boris Stasenko on 07.12.2019
//

#pragma once
#ifndef CLIENT_SERVER_UDP_COMMON_H
#define CLIENT_SERVER_UDP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

enum LOG_TYPE {
    LOG_ONLY,
    DOUBLE_LOG
};

FILE *logger;

void parse(int, char **);

void display_usage(char *);

char *b64_encode(const unsigned char *, size_t);

void p_log(enum LOG_TYPE type, char *format, ...);

#endif //CLIENT_SERVER_UDP_COMMON_H
