#ifndef _SEND_H_
#define _SEND_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


#define SERVER "Made in Jingjun\r\n"
void http_recv(int fd, int events, void *arg);
void http_send(int fd, int events, void *arg);

#endif
