/*
 * hilo.c
 *
 *  Created on: 18 jun. 2022
 *      Author: utnso
 */

/*
 https://sites.ualberta.ca/dept/chemeng/AIX-43/share/man/info/C/a_doc_lib/aixprggd/genprogc/term_threads.htm
 */


#include "../include/thread_monitor.h"
#include <signal.h>
#include <stdio.h>



typedef struct {
	void* (*start_routine)(void *);
	void* restrict arg;
	t_thread_monitor* monitor;
} t_parameter;



static int thread_subscribe(t_thread_monitor* self, pthread_t tid);
static void thread_unsubscribe(t_thread_monitor* self, pthread_t tid);
static void destroy_thread_in_list(void* thread);
static void terminate_and_wait(t_thread_monitor* self);
static int thread_new(t_thread_monitor* self, pthread_t* tid, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
static void execute_thread(void* parameter);
static t_parameter* parameter_create(t_thread_monitor* monitor, void *(*start_routine)(void *), void *restrict arg);
static void parameter_destroy(t_parameter* self);



t_thread_monitor* thread_monitor_create() {
	t_thread_monitor* self = malloc(sizeof(t_thread_monitor));

	pthread_mutex_init(&(self->mutex_monitor), NULL);
	sem_init(&(self->sem_terminated), 0, 0);

	self->active_thread_count = 0;
	self->terminated = false;
	self->threads = list_create();

	self->subscribe = thread_subscribe;
	self->unsubscribe = thread_unsubscribe;
	self->terminate_and_wait = terminate_and_wait;
	self->new = thread_new;

	return self;
}


void thread_monitor_destroy(t_thread_monitor* self) {
	sem_destroy(&(self->sem_terminated));
	pthread_mutex_destroy(&(self->mutex_monitor));

	list_destroy_and_destroy_elements(self->threads, (void*)destroy_thread_in_list);
	free(self);
}




static t_parameter* parameter_create(t_thread_monitor* monitor, void *(*start_routine)(void *), void *restrict arg) {
	t_parameter* self = malloc(sizeof(t_parameter));
	self->arg = arg;
	self->monitor = monitor;
	self->start_routine = start_routine;

	return self;
}

void parameter_destroy(t_parameter* self) {
	free(self);
}


static void destroy_thread_in_list(void* thread) {
	free(thread);
}


static int thread_add(t_thread_monitor* self, pthread_t tid) {
	if (self->terminated) {
		return EXIT_FAILURE;
	}

	// guardo una copia. No se si hace falta esto?
	pthread_t *pnt_thread = malloc(sizeof(pthread_t));
	*pnt_thread = tid;

	list_add(self->threads, pnt_thread);
	self->active_thread_count++;

	return EXIT_SUCCESS;
}

static void thread_remove(t_thread_monitor* self, pthread_t tid) {

	self->active_thread_count--;

	t_list_iterator* iterator = list_iterator_create(self->threads);
	pthread_t *element;
	int index = -1;

	while (list_iterator_has_next(iterator)) {
		element = list_iterator_next(iterator);

		if (pthread_equal(*element, tid) != 0) {
			index = iterator->index;
			break;
		}
	}

	if (index >= 0) {
		pthread_t *ptid = list_remove(self->threads, index);
		free(ptid);
	}

	list_iterator_destroy(iterator);

	if (self->terminated && self->active_thread_count <= 0) {
		sem_post(&(self->sem_terminated));
	}
}


/*
 * @NAME: thread_monitor_add
 * @DESC: metodo thread_safe que agrega un nuevo hilo a la lista para ser monitoreado
 *
 */
static int thread_subscribe(t_thread_monitor* self, pthread_t tid) {
	pthread_mutex_lock(&(self->mutex_monitor));

	int result = thread_add(self, tid);

	pthread_mutex_unlock(&(self->mutex_monitor));

	return result;
}


static void thread_unsubscribe(t_thread_monitor* self, pthread_t tid) {
	pthread_mutex_lock(&(self->mutex_monitor));
	thread_remove(self, tid);
	pthread_mutex_unlock(&(self->mutex_monitor));
}


static int thread_new(t_thread_monitor* self, pthread_t* tid, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg) {

	pthread_mutex_lock(&(self->mutex_monitor));

	if (self->terminated) {
		pthread_mutex_unlock(&(self->mutex_monitor));
		return -1;
	}

	t_parameter* parameter = parameter_create(self, start_routine, arg);

	int result = pthread_create(tid, attr, (void*)execute_thread, (void*)parameter);

	if (result != 0) {
		pthread_mutex_unlock(&(self->mutex_monitor));
		parameter_destroy(parameter);
		return result;
	}

	result = pthread_detach(*tid);

	if (result != 0) {
		pthread_mutex_unlock(&(self->mutex_monitor));
		parameter_destroy(parameter);
		return result;
	}

	thread_add(self, *tid);
	pthread_mutex_unlock(&(self->mutex_monitor));

	return result;
}


static void execute_thread(void* parameter) {
	t_parameter* p = (t_parameter*)parameter;
	pthread_t tid = pthread_self();

	p->start_routine(p->arg);

	pthread_mutex_lock(&(p->monitor->mutex_monitor));
	thread_remove(p->monitor, tid);
	pthread_mutex_unlock(&(p->monitor->mutex_monitor));

	parameter_destroy(p);
}


static void terminate_and_wait(t_thread_monitor* self) {
	pthread_mutex_lock(&(self->mutex_monitor));
	self->terminated = true;
	pthread_mutex_unlock(&(self->mutex_monitor));

	if (self->active_thread_count > 0)
		sem_wait(&(self->sem_terminated));
}
