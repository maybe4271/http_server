#include "epoll.h"
#include "list.h"

//创建链表，返回的是链表的头
url_list* create_list()
{
	url_list *tmp;
	tmp = (url_list *)malloc(sizeof(url_list));
	if(tmp == NULL) {
		perror("url_list malloc error");
		exit(1);
	}
	tmp->next = NULL;
	return tmp;
}

//尾插法插入数据，在外部分配地址空间
void insert_url_list(url_list *head, url_list *tmp)
{
	url_list *phead = head;
	//遍历到链表尾
	while(phead->next != NULL)
		phead = phead->next;

	//将该结点接到链表尾
	phead->next = tmp;
	tmp->next = NULL;
}

//返回链表的长度，包含头节点
int getlen_url_list(url_list *head)
{
	url_list *phead = head->next;
	int i = 0;
	while(phead != NULL) {
		i++;
		phead = phead->next;
	}
	//有头节点
	return i;
}

//删除指定名称的数据
void del_url_list(url_list *head, char *filename)
{
	url_list *phead = head->next;
	url_list *tmp;
	int len = getlen_url_list(head);
	int book = 0;
	
	//如果链表为空
	if(len == 0) {
		printf("删除失败，队列为空\n");
		return;
	}
	
	while(phead != NULL) {
		if(strcmp(phead->filename, filename) == 0) {
			tmp = phead->next;
			phead->next = tmp->next;
			book = 1;
			break;
		}
		phead = phead->next;
	}
	if(book)
		printf("删除成功!\n");
	else
		printf("删除失败，未找到该文件!\n");
}

//反转链表
void reserve_url_list(url_list *head)
{
	if(head->next == NULL) {
		printf("反转失败，链表为空\n");
		return;
	}
	url_list *tmp, *nexttmp;
	url_list *phead = head ->next;
	head->next = NULL;
	nexttmp = NULL;
	while(phead) {
		tmp = phead->next;
		phead->next = nexttmp;
		nexttmp = phead;
		phead = tmp;
	}
	head->next = nexttmp;
	printf("链表反转成功!\n");
}

//打印链表
void print_url_list(url_list *head)
{
	url_list *phead = head->next;
	if(phead == NULL) {
		printf("遍历失败，链表为空！\n");
		return;
	}
	int i = 1;
	while(phead) {
		printf("method:[%s]\n", phead->method);
		printf("filename:[%s]\n", phead->filename);
		printf("type:[%s]\n", phead->type);
		printf("file_length:[%d]\n", phead->file_length);
		printf("index:[%d]\n", i);
		phead = phead->next;
		i++;
	}
	printf("遍历完成!\n");
	printf("\n");
}

url_list* gettail_url_list(url_list *head)
{
	url_list *phead = head->next;
	
	if(phead == NULL) {
		printf("链表为空，获取链表的最后一个元素失败！\n");
		return ;
	}
	while(phead->next != NULL) {
		phead = phead -> next;
	}
	return phead;
}

/*
int main(int argc, char *argv[])
{
	url_list *head, *tmp;
	int i;
	head = create_list();
	for (i = 0; i < 3; i++) {
		printf("请输入信息:\n");
		tmp = (url_list *)malloc(sizeof(url_list));
		scanf("%s", tmp->method);
		scanf("%s", tmp->filename);
		scanf("%s", tmp->type);
		scanf("%d", &(tmp->file_length));
		insert_url_list(head, tmp);
	}
	print_url_list(head);
	reserve_url_list(head);
	print_url_list(head);
	return 0;
}

*/

