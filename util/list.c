#include <stdlib.h>
#include <stdio.h>
#include "list.h"

static dataList_t * _list_new();

static dataList_t * _list_new()
{
	return (dataList_t *)malloc(sizeof(dataList_t));
}

void _list_free(dataList_t *list)
{
	if(list != NULL)
	{
		free(list);
	}
}

int list_add(listInstance_t *inst, void *data)
{
	if(inst == NULL)
	{
		return -1;
	}

	dataList_t *new_list = NULL;
	if((new_list = _list_new()) == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&(inst->mutex));

	new_list->data = data;

	new_list->next = NULL;
	if(inst->head_list == NULL)
	{
		inst->head_list = new_list;
	}
	else
	{
		if(inst->tail_list == NULL)
		{
			printf("err list about tail %s\n", __FUNCTION__);
		}
		(inst->tail_list)->next = new_list;
	}
	inst->tail_list = new_list;

	inst->num++;

	pthread_mutex_unlock(&(inst->mutex));

	return 0;
}

int list_pop(listInstance_t *inst, void **data)
{
	dataList_t *free_list;

	if(inst == NULL)
	{
		return -1;
	}

	if(data == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&(inst->mutex));
	
	if(inst->head_list == NULL)
	{
		if(inst->tail_list != NULL)
		{
			printf("err list about tail %s\n", __FUNCTION__);
		}
		
		pthread_mutex_unlock(&(inst->mutex));
		
		return -1;
	}
	
	*data = (inst->head_list)->data;
	free_list = inst->head_list;
	inst->head_list = (inst->head_list)->next;

	_list_free(free_list);


	if(inst->head_list == NULL)
	{
		inst->tail_list = NULL;
	}

	inst->num--;

	pthread_mutex_unlock(&(inst->mutex));

	return 0;
}

int list_del_all(listInstance_t *inst)
{
	dataList_t *free_list;

	if(inst == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&(inst->mutex));

	while(inst->head_list != NULL)
	{
		free_list = inst->head_list;
		free((inst->head_list)->data);
		inst->head_list = (inst->head_list)->next;
		_list_free(free_list);
	}
	inst->tail_list = NULL;

	inst->num = 0;

	pthread_mutex_unlock(&(inst->mutex));

	return 0;
}

int list_get_num(listInstance_t *inst)
{
	if(inst == NULL)
	{
		return -1;
	}

	return inst->num;
}

