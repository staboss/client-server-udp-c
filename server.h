//
//  Created by Boris Stasenko on 07.12.2019
//

#pragma once
#ifndef CLIENT_SERVER_UDP_SERVER_H
#define CLIENT_SERVER_UDP_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

#define MAX_DATA_UDP 65536

void run_as_daemon(void);

int function_base64(char *, char *);

void server(char *, int, int);

void client(char *, int, char *);

#endif //CLIENT_SERVER_UDP_SERVER_H
