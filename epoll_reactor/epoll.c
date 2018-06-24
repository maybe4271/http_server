#include "epoll.h"
#include "list.h"
#include "send.h"

int g_efd;
http_response_t g_events[MAX_EVENTS+1];

//设置套接字为非阻塞状态
void set_nonblock(int fd)
{
	int res;
	res = fcntl(fd, F_GETFL, 0);
	if(res < 0) {
		perror("fcntl(F_GETFL)");
		exit(1);
	}
	res |= O_NONBLOCK;
	if(fcntl(fd, F_SETFL, res) < 0) {
		perror("fcntl(F_SETFL)");
		exit(1);
	}
}

//注册事件，将回调函数绑定到目标文件描述符上
void eventset(http_response_t* ev, int fd, void (*call_back)(int, int, void*), void *arg)
{
	ev->fd = fd;
	ev->events = 0;
	ev->call_back = call_back;
	ev->arg = arg;

	ev->last_active = time(NULL);
	ev->status = 0;
	if(ev->list_head) {
		ev->list_len = getlen_url_list(ev->list_head);
	}
	return ;
}

//将事件挂到树上，若已经再树上，则只需修改相应的触发事件即可
void eventadd(int efd, int events, http_response_t *ev)
{
	struct epoll_event epv={0, {0}};
	int op = EPOLL_CTL_MOD;
	epv.data.ptr = ev;
	epv.events = ev->events = events;
	
	if(ev->status == 0){
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}
	printf("======================%d\n", ev->fd);
	if(epoll_ctl(g_efd, op, ev->fd, &epv) < 0)
		printf("event add failed [fd=%d], events[%0x]\n", ev->fd, events);
	else
		printf("event add OK [fd=%d], op=%d, events[%0X]\n", ev->fd, op, events);
	
	return ;
}

//将事件从树上摘下来
void eventdel(int efd, http_response_t *ev)
{
	struct epoll_event epv={0,{0}};
	
	if(ev->status != 1)
		return;
	
	//epv.data.ptr = ev;
	epv.data.ptr = NULL;
	ev->status = 0;
	epoll_ctl(g_efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

//epoll_wait监听到客户端请求建立连接的时候，调用该函数与客户端套接字建立连接
void acceptconn(int lfd, int events, void *arg)
{
	struct sockaddr_in cfd_addr;
	socklen_t len = sizeof(cfd_addr);
	int cfd, i;
	
	if((cfd = accept(lfd, (struct sockaddr *)&cfd_addr, &len)) == -1){
		if(errno != EAGAIN && errno != EINTR){
			
		}
		perror("accept error:");
		return ;
	}
	printf("new connect fd:[%d] IP: [%s:%d], pos[%d]\n", cfd, inet_ntoa(cfd_addr.sin_addr),
	 ntohs(cfd_addr.sin_port), i);
	do {
		for(i = 0; i < MAX_EVENTS; i++){
			if(g_events[i].status == 0)
				break;
		}
		
		if(i == MAX_EVENTS){
			perror("connect is max");
			break;
		}
		
		set_nonblock(cfd);
		g_events[i].list_head = create_list();
		eventset(&g_events[i], cfd, http_recv, &g_events[i]);
		eventadd(g_efd, EPOLLIN, &g_events[i]);
	}while(0);
	
	printf("new connect [%s:%d][time:%ld], pos[%d]\n", inet_ntoa(cfd_addr.sin_addr),
	 ntohs(cfd_addr.sin_port), g_events[i].last_active, i);
	
	return ;
}

//初始化服务器端套接字，并绑定到目标端口号
void initlistensocket(int efd, unsigned short port)
{
	struct sockaddr_in lfd_addr;
	int lfd;
	
	lfd = socket(AF_INET, SOCK_STREAM, 0);
	//设置服务器端套接字为非阻塞
	set_nonblock(lfd);
	//设置端口复用
	int reuse = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
	
	
	memset(&lfd_addr, 0, sizeof(lfd_addr));
	lfd_addr.sin_family = AF_INET;
	lfd_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	lfd_addr.sin_port = htons(port);
	
	if((bind(lfd, (struct sockaddr *)&lfd_addr, sizeof(lfd_addr)) < 0))
		perror("bind:");
		
	if((listen(lfd, 100)) < 0)
		perror("listen:");
	
	eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
	eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);
	
	return ;
}

//测试用main函数
int main(int argc, char* argv[])
{
	unsigned short port = PORT;
	if(argc == 2)
		port = atoi(argv[1]);

	g_efd = epoll_create(MAX_EVENTS+1);
	if(g_efd <= 0)
		perror("g_efd create error:");

	initlistensocket(g_efd, port);
	
	struct epoll_event events[MAX_EVENTS+1];
	
	printf("server running......\nport %d\n", port);
	
	int checkpos = 0, i;
	
	while(1) {
		//心跳模块
		long now = time(NULL);
		printf("time is %ld\n", now);
		for(i = 0;i < 100; i++, checkpos++)	{
			if(checkpos == MAX_EVENTS)
				checkpos = 0;
			if(g_events[checkpos].status != 1)
				continue;
				
			long duration = 0;
			
			if(duration >= 60){
				close(g_events[checkpos].fd);
				
				printf("Client %d timeout\n", g_events[checkpos].fd);
				eventdel(g_efd, &g_events[checkpos]);
			}
		}
		
		int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, -1);
		if(nfd < 0) {
			printf("epoll_wait error, exit\n");
			break;
		}
		
		for(i = 0; i < nfd; i++) {
			http_response_t *ev = (http_response_t *)events[i].data.ptr;
			
			if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
				ev->call_back(ev->fd, events[i].events, ev->arg);
			}
			if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
				ev->call_back(ev->fd, events[i].events, ev->arg);
			}
		}
	}
	
	return 0;
}

