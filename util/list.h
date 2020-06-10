<<<<<<< HEAD
#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

#include <pthread.h>

#define list_for_each(pos, head) \
	for(pos = head; pos != 0; pos = pos->next)

typedef struct dataList dataList_t;
struct dataList
{
	void *data;
	dataList_t *next;
};

typedef struct listInstance listInstance_t;
struct listInstance
{
	dataList_t *head_list;
	dataList_t *tail_list;
	pthread_mutex_t mutex;
	int num;
};

void _list_free(dataList_t *list);
int list_add(listInstance_t *inst, void *data);
int list_pop(listInstance_t *inst, void **data);
int list_del_all(listInstance_t *inst);
int list_get_num(listInstance_t *inst);

#endif
=======
#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

#include <pthread.h>

#define list_for_each(pos, head) \
	for(pos = head; pos != 0; pos = pos->next)

typedef struct dataList dataList_t;
struct dataList
{
	void *data;
	dataList_t *next;
};

typedef struct listInstance listInstance_t;
struct listInstance
{
	dataList_t *head_list;
	dataList_t *tail_list;
	pthread_mutex_t mutex;
	int num;
};

void _list_free(dataList_t *list);
int list_add(listInstance_t *inst, void *data);
int list_pop(listInstance_t *inst, void **data);
int list_del_all(listInstance_t *inst);
int list_get_num(listInstance_t *inst);

#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
