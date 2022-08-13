/*
 * safe_queue.c
 *
 *  Created on: 12 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>

#include "../include/squeue.h"



t_squeue* squeue_create() {
	t_squeue* self = malloc(sizeof(t_squeue));
	pthread_mutex_init(&(self->mutex), NULL);
	self->queue = queue_create();

	return self;
}

void squeue_destroy(t_squeue* self) {
	pthread_mutex_destroy(&(self->mutex));
	queue_destroy(self->queue);
	free(self);
}


void squeue_destroy_and_destroy_elements(t_squeue* self, void(*element_destroyer)(void*)) {
	queue_clean_and_destroy_elements(self->queue, element_destroyer);
	squeue_destroy(self);
}


void* squeue_remove(t_squeue* self, void* val, bool comparer(void*, void*)) {
	pthread_mutex_lock(&(self->mutex));

	bool match = false;
	void* element = NULL;
	t_list_iterator* iterator = list_iterator_create(self->queue->elements);

	while(!match && list_iterator_has_next(iterator)) {
		element = list_iterator_next(iterator);
		match = comparer(val, element);
	}

	if (match) {
		list_remove(self->queue->elements, iterator->index);
	}

	list_iterator_destroy(iterator);

	pthread_mutex_unlock(&(self->mutex));

	if (match)
		return element;
	else
		return NULL;
}


int squeue_size(t_squeue* self) {
	int size = 0;

	pthread_mutex_lock(&(self->mutex));

	if (!queue_is_empty(self->queue))
		size = queue_size(self->queue);

	pthread_mutex_unlock(&(self->mutex));

	return size;
}


bool squeue_is_empty(t_squeue* self) {
	pthread_mutex_lock(&(self->mutex));
	bool empty = queue_is_empty(self->queue);
	pthread_mutex_unlock(&(self->mutex));

	return empty;
}

void* squeue_peek(t_squeue* self) {
	pthread_mutex_lock(&(self->mutex));
	void* element = queue_peek(self->queue);
	pthread_mutex_unlock(&(self->mutex));

	return element;
}

void* squeue_pop(t_squeue* self) {
	pthread_mutex_lock(&(self->mutex));
	void* element = queue_pop(self->queue);
	pthread_mutex_unlock(&(self->mutex));

	return element;
}


void squeue_push(t_squeue* self, void* element) {
	pthread_mutex_lock(&(self->mutex));
	queue_push(self->queue, element);
	pthread_mutex_unlock(&(self->mutex));
}


void squeue_insert(t_squeue* self, void* element, int index) {
	pthread_mutex_lock(&(self->mutex));
	list_add_in_index(self->queue->elements, index, element);
	pthread_mutex_unlock(&(self->mutex));
}


void squeue_insert_in_order(t_squeue* self, void* element, int (*comparer)(void*, void*)) {
	pthread_mutex_lock(&(self->mutex));

	int index = -1;
	void* element_in_list = NULL;

	if (queue_is_empty(self->queue)) {
		queue_push(self->queue, element);
	}
	else {
		t_list_iterator* iterator = list_iterator_create(self->queue->elements);

		while(list_iterator_has_next(iterator)) {
			element_in_list = list_iterator_next(iterator);

			// corto cuando estoy en la posicion en la que el nuevo elemento tiene mayor prioridad que los que siguen en la lista
			if (comparer(element, element_in_list) == 1) {
				index = iterator->index;
				break;
			}
		}

		//printf("squeue_insert_in_order :: index:[%d]\n", index);

		if (index >= 0)
			list_add_in_index(self->queue->elements, index, element);
		else
			queue_push(self->queue, element);

		list_iterator_destroy(iterator);
	}

	//printf("squeue_insert_in_order :: insertado\n");

	pthread_mutex_unlock(&(self->mutex));
}
