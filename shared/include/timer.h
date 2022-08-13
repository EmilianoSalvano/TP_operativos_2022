/*
 * timer.h
 *
 *  Created on: 15 jul. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_TIMER_H_
#define INCLUDE_TIMER_H_


#include <pthread.h>
#include <stdbool.h>

typedef struct {
	pthread_t				tid;		// read only
	int						interval;	// read only
	//struct timespec			max_wait;	// read only
	void*					data;
	bool					canceled;	// read only
	int						error;		// read only
	//pthread_mutex_t			mutex;		// read only
	//pthread_cond_t			condition;	// read only
	void					(*callback)(void* data);
} t_timer;


t_timer* timer_create_t(int interval);
void timer_destroy(t_timer* self);
void timer_cancel(t_timer* self);
int timer_start(t_timer* self);


#endif /* INCLUDE_TIMER_H_ */
