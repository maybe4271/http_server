#include "send.h"
#include "epoll.h"
#include "list.h"

extern int g_efd; //epoll的根节点，在epoll.c里面定义
extern http_response_t g_events[MAX_EVENTS+1];

void judge_type(char *type)
{
	
}

//send 200 OK
int file_OK(int fd, long flen, char *filetype)
{
	char send_buf[BUFSIZE*10];
	sprintf(send_buf, "HTTP/1.1 200 OK\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "Connection:keep-alive\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, SERVER);
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "Content-Length:%ld\r\n", flen);
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "Content-Type:%s\r\n", filetype);
	printf("fileType : %s\n" , filetype);
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	printf("build head fd : %d\n" , fd);
	return 1;
}

//send 404 not found
int file_not_found(int fd)
{
	char send_buf[BUFSIZE*10];
	sprintf(send_buf, "HTTP/1.1 404 NOT FOUND\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "Connection:keep-alive\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, SERVER);
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "Content-Type:text/html\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	return 1;
}

//send 404 page from myself
int send_not_found(int fd)
{
	char send_buf[BUFSIZE*10];
	sprintf(send_buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "<BODY><h1 align='center'>404</h1><br/><h1 align='center'>file not found.</h1>\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	sprintf(send_buf, "</BODY></HTML>\r\n");
	send(fd, send_buf, strlen(send_buf), 0);
	printf("404 page send OK!\n");
	return 1;
}

//send file
int send_file(int fd, FILE *fp)
{
	char send_buf[BUFSIZE*10];
	int data_len;
	memset(send_buf, 0, sizeof(send_buf));
	while((data_len = fread(send_buf, sizeof(char), BUFSIZE, fp)) > 0)
	{
		printf("===========data_len = %d\n", data_len);
		send(fd, send_buf, data_len, 0);
		memset(send_buf, 0, sizeof(send_buf));
	}
	printf("file send ok!\n");
	fclose(fp);
	
	return 1;
}


//接受客户端传来的信息
void http_recv(int fd, int events, void *arg)
{
	http_response_t *ev = (http_response_t *)arg;
	int n, i = 0, j = 0;
	char recv_buf[BUFSIZE*10];
	char method[5];
	char url[100];
	char type[30];
	
	url_list *tmp;
	tmp = (url_list *)malloc(sizeof(url_list));
	if(tmp == NULL) {
		perror("tmp malloc error");
		return;
	}
	//清空缓冲区
	memset(recv_buf, 0, sizeof(recv_buf));
	n = recv(ev->fd, recv_buf, sizeof(recv_buf), 0);
	
	eventdel(g_efd, ev);
	//将我们需要的信息从请求报文中提取出来，入method，url等
	//printf("%s\n", recv_buf);
	if(n > 0) {
		printf("recv ok!\n");
		while(recv_buf[i]!=' ') {
			method[i] = recv_buf[i];
			i++;
		}
		method[i] = '\0';
		strcpy(tmp->method, method);
		i = i+1;
		while(recv_buf[i] != ' ' && j < 128) {
			url[j] = recv_buf[i];
			i++; j++;
		}
		url[j] = '\0';
		strcpy(tmp->filename, url);
		i = 0;
		while(url[i] != '.')
			i++;
		j = 0;
		while(url[i] != '\0') {
			type[j] = url[i];
			i++; j++;
		}
		//根据请求的文件的后缀判断Content-Type
		if(strcmp(type, ".html") == 0 || strcmp(type, ".htm") == 0)
			strcpy(tmp->type, "text/html");
		else if(strcmp(type, ".txt") == 0)
			strcpy(tmp->type, "text/plain");
		else
			printf("type:%s\n" , type);

		//将信息保存到链表的尾部
		insert_url_list(ev->list_head, tmp);
		printf("fd:%d\n", ev->fd);
		printf("method:%s\n", tmp->method);
		printf("url:%s\n", tmp->filename);
		
		//注册并添加事件到树上
		eventset(ev, fd, http_send, ev);
		eventadd(g_efd, EPOLLOUT, ev);
	} else if(n == 0) {
		printf("client:%d is closed\n", fd);
		return;
	} else {
		perror("recv error");
		return;
	}
}

//发送事件
void http_send(int fd, int events, void *arg)
{
	printf("send file fd : %d\n" , fd);
	http_response_t *ev = (http_response_t *)arg;
	url_list *tmp = gettail_url_list(ev->list_head);
	char path[BUFSIZE];
	
	printf("fd:%d\n", ev->fd);
	printf("method:%s\n", tmp->method);
	printf("url:%s\n", tmp->filename);
	
	eventdel(g_efd, ev);
	
	//如果不是GET或HEAD方法，则断开本次连接，可以返回一个501未实现的报头页面
	if(strcmp(tmp->method, "GET") && strcmp(tmp->method, "HEAD")) {
		printf("not get or head method\nclose ok.\n");
		close(ev->fd);
		eventdel(g_efd, ev);

		exit(1);
	}
	//获取本地项目目录
	if(getcwd(path, sizeof(path)) == NULL) {
		printf("getcwd error!\n");
	}
	strcat(path, tmp->filename);
	printf("path is [%s]\n", path);
	//打开客户端请求的文件
	FILE *resource = fopen(path, "rb");
	if(resource == NULL) {
		file_not_found(ev->fd);
		if(strcmp(tmp->method, "GET") == 0) {
			send_not_found(ev->fd);
		}
		close(ev->fd);
		fclose(resource);
		eventdel(g_efd, ev);
		printf("page was not found\nclose ok.\n");
		exit(1);
	}
	fseek(resource, 0, SEEK_END);
	long flen = ftell(resource);
	printf("file length: %ld\n", flen);
	fseek(resource, 0, SEEK_SET);
	//如果是GET的请求方式，则发送文件
	if(strcmp(tmp->method, "GET") == 0) {
		file_OK(ev->fd, flen, tmp->type);
		send_file(ev->fd, resource);
	}
	
	//将事件注册到回调函数上，并将事件挂到树上
	eventset(ev, fd, http_recv, ev);
	eventadd(g_efd, EPOLLIN, ev);
}


