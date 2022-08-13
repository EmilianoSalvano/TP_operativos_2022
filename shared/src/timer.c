/*
 * timer.c
 *
 *  Created on: 15 jul. 2022
 *      Author: utnso
 */
#include <stdlib.h>
//#include <signal.h>
#include <unistd.h>
#include "../include/timer.h"
#include "../include/kiss.error.h"

//#include <stdio.h>
//#include <stdint.h>
//#include <inttypes.h>
//#include "../include/time_utils.h"

// https://bytefreaks.net/programming-2/c-full-example-of-pthread_cond_timedwait

static void timer_sleep(void* timer);


t_timer* timer_create_t(int interval) {
	t_timer* self = malloc(sizeof(t_timer));
	self->tid = -1;
	self->interval = interval;
	self->callback = NULL;
	self->data = NULL;
	self->canceled = false;
	self->error = 0;
	//self->max_wait.tv_sec = 0;
	//self->max_wait.tv_nsec = 0;

	return self;
}

void timer_destroy(t_timer* self) {
	free(self);
}

void timer_cancel(t_timer* self) {
	self->canceled = true;
}


int timer_start(t_timer* self) {
	if (pthread_create(&(self->tid), NULL, (void*)timer_sleep, (void*)self) != 0) {
		self->error = errno;
		return EXIT_FAILURE;
	}
	else {
		if (pthread_detach(self->tid) != 0) {
			self->error = errno;
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}
}


static void timer_sleep(void* timer) {
	t_timer* self = (t_timer*)timer;

	if (usleep(self->interval * 1000) != 0) {
		self->error = errno;
		return;
	}

	if (!self->canceled)
		self->callback(self->data);

	timer_destroy(self);
}


/*
static void timer_sleep(void* timer) {
	t_timer* self = (t_timer*)timer;

	//int oldtype;
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	clock_gettime(CLOCK_REALTIME, &(self->max_wait));
	self->max_wait.tv_sec += self->interval / 1000;
	self->max_wait.tv_nsec += (self->interval % 1000) * 1000000;

	uint64_t start_stamp = get_time_stamp();

	pthread_mutex_lock(&(self->mutex));
	pthread_cond_timedwait(&(self->condition), &(self->mutex), &(self->max_wait));

	uint64_t end_stamp = get_time_stamp();
	printf("timer_sleep :: real time elapsed %"PRId64" milliseconds\n", (end_stamp - start_stamp));

	if (!self->canceled)
		self->callback(self);
}


void timer_start(t_timer* self) {

	pthread_cond_init(&(self->condition), NULL);
	pthread_mutex_init(&(self->mutex), NULL);
	//pthread_mutex_lock(&(self->mutex));

	pthread_create(&(self->tid), NULL, (void*)timer_sleep, (void*)self);
	pthread_detach(self->tid);

}


int timer_cancel(t_timer* self) {
	if (self->tid == -1)
		return EXIT_SUCCESS;

	pthread_cond_t condition = self->condition;
	self->canceled = true;
	return pthread_cond_signal(&condition);
}



void timer_start(t_timer* timer) {
	pthread_create(&(timer->tid), NULL, (void*)timer_sleep, (void*)timer);
	pthread_detach(timer->tid);
}

static void timer_sleep(t_timer* self) {

	if (usleep(self->interval * 1000) == 0) {
		self->tid = -1;
		self->callback(self);
	}
	else {
		self->tid = -1;
	}

	pthread_exit(NULL);
}
*/
