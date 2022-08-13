/*
 * main.c
 *
 *  Created on: 1 ago. 2022
 *      Author: utnso
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "../include/main.h"
#include "../include/console_producer.h"

#include "../include/memoriaswap.h"
#include "../include/cpu.h"
#include "../include/kernel.h"
#include "../include/console.h"


static void run_process(void* (*start_proc)(void*), void* log_level);
static void void_memoria_iniciar(void* l);
static void void_cpu_iniciar(void* l);
static void void_kernel_execute(void* l);
static void void_mock_console_run(void* l);

int main() {

	printf("proceso: %d\n", getpid());

	remove("kernel.log");
	remove("cpu.log");
	remove("memoria.log");

	t_log_level l = LOG_LEVEL_INFO;

	run_process((void*)void_memoria_iniciar, (void*)l);
	usleep(2000000);

	run_process((void*)void_cpu_iniciar, (void*)l);
	usleep(2000000);

	run_process((void*)void_kernel_execute, (void*)l);
	usleep(2000000);

	run_process((void*)void_mock_console_run, (void*)l);

	pthread_exit(NULL);
}


static void run_process(void* (*start_proc)(), void* log_level) {
	pthread_t tid;

	pthread_create(&tid, NULL, start_proc, log_level);
	pthread_detach(tid);
}


static void void_memoria_iniciar(void* l) {
	memoria_iniciar((t_log_level)l);
}

static void void_cpu_iniciar(void* l) {
	cpu_iniciar((t_log_level)l);
}

static void void_kernel_execute(void* l) {
	kernel_execute((t_log_level)l);
}

static void void_mock_console_run(void* l) {
	mock_console_run();
}



