#ifndef GRASS_H
#define GRASS_H

#define DEBUG true

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>

struct User {
    const char* uname;
    const char* pass;

    bool isLoggedIn;
};

struct Command {
    const char* cname;
    const char* cmd;
    const char* params;
};

void hijack_flow();

#endif
