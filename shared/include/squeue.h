/*
 * safe_queue.h
 *
 *  Created on: 12 jul. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_SQUEUE_H_
#define INCLUDE_SQUEUE_H_

#include <pthread.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>


typedef struct {
	t_queue*		queue;
	pthread_mutex_t	mutex;
} t_squeue;


t_squeue* squeue_create();
void squeue_destroy(t_squeue* self);
void squeue_destroy_and_destroy_elements(t_squeue* self, void(*element_destroyer)(void*));


int squeue_size(t_squeue* self);
bool squeue_is_empty(t_squeue* self);
void* squeue_peek(t_squeue* self);
void* squeue_pop(t_squeue* squeue);
void squeue_push(t_squeue* squeue, void* element);
void* squeue_remove(t_squeue* squeue, void* val, bool comparer(void*, void*));
void squeue_insert(t_squeue* squeue, void* element, int index);
void squeue_insert_in_order(t_squeue* squeue, void* element, int (*comparer)(void*, void*));



#endif /* INCLUDE_SQUEUE_H_ */
