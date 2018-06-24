#ifndef _LIST_H_
#define _LIST_H_
#include "epoll.h"

url_list* create_list(void);
void insert_url_list(url_list *head, url_list *tmp);
int getlen_url_list(url_list *head);
void del_url_list(url_list *head, char *filename);
void reserve_url_list(url_list *head);
void print_url_list(url_list *head);
url_list* gettail_url_list(url_list *head);

#endif
