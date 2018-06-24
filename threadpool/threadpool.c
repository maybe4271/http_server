#include "threadpool.h"

//创建线程池
threadpool_t* threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
	threadpool_t *pool;
	do{
		if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == 0) {
			perror("malloc error");
			break;
		}
		
		pool->min_thr_num = min_thr_num;
		pool->max_thr_num = max_thr_num;
		pool->queue_max_size = queue_max_size;
		pool->busy_thr_num = 0;
		pool->live_thr_num = min_thr_num;
		pool->wait_exit_thr_num = 0;
		pool->queue_size = 0;
		pool->queue_front = 0;
		pool->queue_rear = 0;
		pool->shutdown = 0;
		
		
		if((pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*max_thr_num)) == 0) {
			perror("pool->threads malloc error");
			break;
		}
		
		if((pool->taskqueue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size)) == 0) {
			perror("adjust thread malloc error");
			break;
		}
		
		
		if(pthread_mutex_init(&(pool->lock), NULL) != 0 || pthread_mutex_init(&(pool->thread_count), NULL) != 0 ||
			pthread_cond_init(&(pool->queue_not_empty), NULL) != 0 || pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
			perror("mutex lock or condition init error");
			break;
		}
		
		int i;
		for(i = 0; i < min_thr_num; i++) {
			//创建线程池中的任务线程
			pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
			printf("thread id[0x%x] create successfully\n", (unsigned int)pool->threads[i]);
		}
		//创建管理者线程
		pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);
		printf("adjust thread: 0x%x create successfully", (unsigned int)pool->adjust_tid);
		return pool;
		
	} while(0);

	threadpool_free(pool);
	return NULL;
}
//添加线程
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg), void *arg)
{
	pthread_mutex_lock(&(pool->lock));
	//当队列已满，且shutdown为0
	while((pool->queue_size == pool->queue_max_size) && shutdown == 0)
		pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
	
	if(pool->shutdown == 1)
		pthread_mutex_unlock(&(pool->lock));
	//清空回调函数的参数
	if(pool->taskqueue[pool->queue_rear].arg != NULL) {
		free(pool->taskqueue[pool->queue_rear].arg);
		pool->taskqueue[pool->queue_rear].arg = NULL;
	}
	//将任务加入到任务队列中
	pool->taskqueue[pool->queue_rear].function = function;
	pool->taskqueue[pool->queue_rear].arg = arg;
	pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
	pool->queue_size++;
	//唤醒线程池中的线程
	pthread_cond_signal(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->lock));
	
	return 0;
}

void *threadpool_thread(void *threadpool)
{
	threadpool_t *pool = (threadpool_t *)threadpool;
	threadpool_task_t task;
	
	while(1)
	{
		pthread_mutex_lock(&(pool->lock));
		while((pool->queue_size == 0) && pool->shutdown == 0) {
			printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
			pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
			
			if(pool->wait_exit_thr_num > 0) {
				pool->wait_exit_thr_num--;
				
				if(pool->live_thr_num > pool->min_thr_num) {
					printf("thread 0x%x is end\n", (unsigned int)pthread_self());
					pool->live_thr_num--;
					pthread_mutex_unlock(&(pool->lock));
					pthread_exit(NULL);
				}
			}
		}
		
		if(pool->shutdown == 1) {
			printf("thread 0x%x is end\n", (unsigned int)pthread_self());
			pool->live_thr_num--;
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		
		task.function = pool->taskqueue[pool->queue_front].function;
		task.arg = pool->taskqueue[pool->queue_front].arg;
		
		pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
		pool->queue_size--;
		
		pthread_cond_broadcast(&(pool->queue_not_full));
		pthread_mutex_unlock(&(pool->lock));
		
		printf("thread 0x%x start working\n", (unsigned int)pthread_self());

		pthread_mutex_lock(&(pool->thread_count));
		pool->busy_thr_num++;
		pthread_mutex_unlock(&(pool->thread_count));
		
		(*(task.function))(task.arg);
		
		pthread_mutex_lock(&(pool->thread_count));
		pool->busy_thr_num--;
		pthread_mutex_unlock(&(pool->thread_count));
		printf("thread 0x%x end working\n", (unsigned int)pthread_self());
	}
	pthread_exit(NULL);
}

void *adjust_thread(void *threadpool)
{
	int i;
	threadpool_t *pool = (threadpool_t *)threadpool;
	while(pool->shutdown == 0) {
		sleep(DEFAULT_TIME);
		pthread_mutex_lock(&(pool->lock));
		int queue_size = pool->queue_size;
		int live_thr_num = pool->live_thr_num;
		
		pthread_mutex_lock(&(pool->thread_count));
		int busy_thr_num = pool->busy_thr_num;
		pthread_mutex_unlock(&(pool->thread_count));
		
		if(queue_size >= pool->min_thr_num && live_thr_num < pool->max_thr_num) {
			int add = 0;
			for(i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY && pool->live_thr_num < pool->max_thr_num; i++) {
				if(pool->threads[i] == 0 || is_thread_alive(pool->threads[i]) == 0) {
					pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
					add++;
					pool->live_thr_num++;
				}
			}
		}
		
		if((busy_thr_num * 2) < live_thr_num && live_thr_num > pool->min_thr_num) {
			pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
			for(i = 0; i < DEFAULT_THREAD_VARY; i++) {
				pthread_cond_signal(&(pool->queue_not_empty));
			}
		}
		
		pthread_mutex_unlock(&(pool->lock));
	}
	return NULL;
}

int threadpool_destory(threadpool_t *pool)
{
	int i;
	if(pool == NULL)
		return -1;
	
	pool->shutdown = 1;
	pthread_join(pool->adjust_tid, NULL);
	for(i = 0; i < pool->live_thr_num; i++)
		pthread_cond_broadcast(&(pool->queue_not_empty));

	for(i = 0; i < pool->live_thr_num; i++)
		pthread_join(pool->threads[i], NULL);

	threadpool_free(pool);
	return 0;
}

int threadpool_free(threadpool_t *pool)
{
	if(pool == NULL)
		return -1;
	
	if(pool->taskqueue)
		free(pool->taskqueue);
	
	if(pool->threads) {
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_mutex_lock(&(pool->thread_count));
		pthread_mutex_destroy(&(pool->thread_count));
		pthread_cond_destroy(&(pool->queue_not_empty));
		pthread_cond_destroy(&(pool->queue_not_full));
	}
	free(pool);
	pool=NULL;
	return 0;
}


int is_thread_alive(pthread_t thread)
{
	int kill_rc = pthread_kill(thread, 0);
	if(kill_rc == ESRCH)
		return 0;
	return 1;
}

void *process(void *arg)
{
	printf("thread 0x%x working on task %d\n ",(unsigned int)pthread_self(),*(int *)arg);
	sleep(1);
	printf("thread 0x%x worke end %d\n ",(unsigned int)pthread_self(),*(int *)arg);
	return NULL;
}

int main()
{
	threadpool_t *pool = threadpool_create(3, 100, 100);
	int i;
	for(i = 0; i < 20; i++) {
		threadpool_add(pool, process, (void *)&i);
	}
	sleep(10);
	threadpool_destory(pool);
	return 0;
}






