/*
 * timer.test.c
 *
 *  Created on: 15 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <syscall.h>

#include "../include/timer.test.h"
#include "../include/test.h"
#include "../../shared/include/time_utils.h"

t_test* test;
sem_t sem_timer;

uint64_t start_stamp;

void timer_test() {
	test = test_open("timer_test");

	sem_init(&sem_timer, 0, 0);
	clock_t start = clock();
	test_fprint(test, "timer_test :: start = %ld\n", start);

	start_stamp = get_time_stamp();
	test_fprint(test, "timer_test :: start_stamp = %"PRId64"\n", start_stamp);

	t_timer* timer = timer_create_t(3000);
	timer->callback = on_timer;
	timer->data = (void*)start;
	timer_start(timer);

	sem_wait(&sem_timer);
	sem_destroy(&sem_timer);

	test_close(test);
}


void timer_cancel_test() {
	test = test_open("timer_test");
	test_fprint(test, "timer_test :: hilo principal = %lu\n", pthread_self());
	printf("tid = %lu\n", syscall(SYS_gettid));

	clock_t start = clock();
	test_fprint(test, "timer_test :: start = %ld\n", start);

	start_stamp = get_time_stamp();
	test_fprint(test, "timer_test :: start_stamp = %"PRId64"\n", start_stamp);

	t_timer* timer = timer_create_t(10000);
	timer->callback = on_timer;
	timer->data = (void*)start;
	timer_start(timer);

	test_fprint(test, "timer_test :: hilo timer = %lu\n", timer->tid);

	// le doy tiempo a que inicie el timer. Este tiempo no debe superar el asignado al intervalo
	usleep(1000000);
	timer_cancel(timer);
	test_fprint(test, "timer_test :: timer cancelado\n");

	// un poco de tiempo para
	usleep(1000000);
	timer_destroy(timer);

	test_close(test);

	pthread_exit(NULL);
}



void on_timer(t_timer* timer) {
	clock_t end = clock();
	uint64_t end_stamp = get_time_stamp();
	clock_t start = (clock_t)(timer->data);

	test_fprint(test, "on_timer :: end = %ld\n", end);
	test_fprint(test, "on_timer :: end_stamp = %"PRId64"\n", end_stamp);

	clock_t difference = end - start;
	double msec = ((double)difference / CLOCKS_PER_SEC) * 1000;

	test_fprint(test, "on_timer :: cpu time elapsed %f milliseconds\n", msec);
	test_fprint(test, "on_timer :: real time elapsed %"PRId64" milliseconds\n", (end_stamp - start_stamp));
	timer_destroy(timer);

	sem_post(&sem_timer);
}
