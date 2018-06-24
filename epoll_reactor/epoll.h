#ifndef _EPOLL_H_
#define _EPOLL_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

//保存客户端传来的数据的链表的结构体
typedef struct node {
	//char buf[10240];
	char method[5];
	char filename[100];
	char type[30];
	int file_length;
	//char cookies[1000];
	struct node *next;
}url_list;


//
typedef struct HttpResponse {
	int fd; //文件描述符
	int events; //fd所监听的事件
	void *arg; //回调函数的参数
	void (*call_back)(int fd, int events, void *arg);//回调函数
	
	int status;//判断该文件描述符是否在树上 1 Yes 0 No
	
	long last_active; //客户端最近一次的触发事件的时间
	
	url_list *list_head; //保存客户端传来的数据的链表头
	int list_len; //链表的长度
}http_response_t;

#define MAX_SIZE 1000 //最多保存1000条记录
#define MAX_EVENTS 1024 //最大事件数
#define BUFSIZE 1024 //缓冲区的大小
#define PORT 8080 //端口号

void set_nonblock(int fd);
void eventset(http_response_t* ev, int fd, void (*call_back)(int, int, void*), void *arg);
void eventadd(int efd, int events, http_response_t *ev);
void eventdel(int efd, http_response_t *ev);
void acceptconn(int lfd, int events, void *arg);
void initlistensocket(int efd, unsigned short port);












#endif
