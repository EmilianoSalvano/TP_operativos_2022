/*
 * monitor.test.c
 *
 *  Created on: 27 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/process.h>

#include "../include/monitor.test.h"
#include "../../shared/include/thread_monitor.h"

static void execute();
static void subscribe_execute();

void test_monitor() {
	t_thread_monitor* monitor = thread_monitor_create();
	pthread_t tid;
	monitor->new(monitor, &tid, NULL, (void*)execute, NULL);
	monitor->new(monitor, &tid, NULL, (void*)execute, NULL);

	printf("monitor: espero a que se ejecuten los hilos\n");
	monitor->terminate_and_wait(monitor);

	printf("monitor: hilos ejecutados\n");
	thread_monitor_destroy(monitor);
}

static void execute() {
	pthread_t tid = pthread_self();
	unsigned long id = process_get_thread_id();

	printf("Inicio hilo:[%lu] id:[%lu]\n", (unsigned long)tid, (unsigned long)id);
	usleep(3000000);
	printf("Fin hilo:[%lu] id:[%lu]\n", (unsigned long)tid, (unsigned long)id);
}



void test_subscribe_monitor() {
	t_thread_monitor* monitor = thread_monitor_create();

	pthread_t tid;
	pthread_create(&tid, NULL, (void*) subscribe_execute, (void*)monitor);
	pthread_detach(tid);

	pthread_create(&tid, NULL, (void*) subscribe_execute, (void*)monitor);
	pthread_detach(tid);

	printf("monitor: espero a que se ejecuten los hilos\n");
	monitor->terminate_and_wait(monitor);

	printf("monitor: hilos ejecutados\n");
	thread_monitor_destroy(monitor);
}

static void subscribe_execute(void* mon) {
	t_thread_monitor* monitor = (t_thread_monitor*)mon;
	pthread_t tid = pthread_self();
	unsigned long id = process_get_thread_id();

	monitor->subscribe(monitor, tid);

	printf("Inicio hilo:[%lu] id:[%lu]\n", (unsigned long)tid, (unsigned long)id);
	usleep(3000000);
	printf("Fin hilo:[%lu] id:[%lu]\n", (unsigned long)tid, (unsigned long)id);

	monitor->unsubscribe(monitor, tid);
}



