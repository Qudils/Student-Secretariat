#ifndef WRAPPER_WRAPPER_H
#define WRAPPER_WRAPPER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include<errno.h>

ssize_t FullRead(int, void *, size_t);

ssize_t FullWrite(int, const void *, size_t);

int Socket(int, int, int);

void Bind(int, const struct sockaddr *, socklen_t);

void Ascolta(int, int);

void Connetti(int, const struct sockaddr *, socklen_t);

int Accetta(int, struct sockaddr *, socklen_t *);

#endif //WRAPPER_WRAPPER_H