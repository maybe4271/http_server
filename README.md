文件结构
.
├── epoll.c
├── epoll.h
├── index.html
├── list.c
├── list.h
├── makefile
├── READEME.md
├── send.c
├── send.h
└── test.txt

1、如何运行此项目？
在项目目录下打开终端，

make
./http

即可运行，然后在浏览器中输入127.0.0.1:8080/index.html即可看到文件下载服务器的主界面，点击目标txt文件下载即可。

2、整体思路：
是单线程单epoll+reactor的文件下载服务器，暂时只有一个txt文件可下载。具体执行过程是，客户端请求连接，服务器端触发EPOLLIN事件，调用相应的回调函数（acceptconn函数）;
客户端发送请求报文，服务器端触发EPOLLIN事件，调用相应的回调函数（http_recv函数），将事件从树上摘下，处理请求，绑定相应的事件（http_send函数），将事件添加到树上;服
务器端触发EPOLLOUT事件，调用相应的回调函数（http_send函数），将事件从树上摘下，发送数据，绑定相应的事件（http_recv函数），将事件添加到树上。



epoll.h和epoll.c是epoll模块。
list.h和list.c是链表的操作。
send.h和send.c是发送具体数据的模块。

主要结构体：
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
