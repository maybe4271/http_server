#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#define DEFAULT_TIME 10
#define DEFAULT_THREAD_VARY 10

//回调函数
typedef struct func {
	void *arg;
	void* (*function)(void *arg);
}threadpool_task_t;


typedef struct threadpool_t {
	pthread_mutex_t lock;//用于锁住整个结构体
	pthread_mutex_t thread_count;//用于锁住忙线程数
	pthread_cond_t queue_not_empty;//任务队列不为空
	pthread_cond_t queue_not_full;//任务队列不为满
	
	pthread_t *threads;//保存线程池线程id
	pthread_t adjust_tid;//管理者线程id
	threadpool_task_t *taskqueue;//任务队列
	
	int min_thr_num;//最小线程数
	int max_thr_num;//最大线程数
	int busy_thr_num;//忙线程数
	int live_thr_num;//活着的线程数
	int wait_exit_thr_num;//等待被销毁的线程
	
	int queue_front;//任务队列头
	int queue_rear;//任务队列尾
	int queue_size;//任务队列大小
	int queue_max_size;//任务队列中任务最大数目
	
	int shutdown; //是否关闭线程池 0 NO 1 YES
}threadpool_t;

threadpool_t* threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size);//创建线程池
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg), void *arg);//向线程池中添加任务
void *threadpool_thread(void *arg);//线程池线程
void *adjust_thread(void *arg);//管理者线程
int threadpool_destory(threadpool_t *pool);//销毁线程池
int threadpool_free(threadpool_t *pool);//释放线程池内存和锁内存
int is_thread_alive(pthread_t thread);//检测线程是否活着
void *process(void *arg);

#endif
