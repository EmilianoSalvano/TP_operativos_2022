/*
 * simple.socket.test.c
 *
 *  Created on: 24 jun. 2022
 *      Author: utnso
 */

/*
 * ps -T -p <pid>
 *
 *
 * error 98
 * https://handsonnetworkprogramming.com/articles/bind-error-98-eaddrinuse-10048-wsaeaddrinuse-address-already-in-use/
 */


#include "../include/simple.socket.test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <commons/string.h>
#include "../../shared/include/conexiones.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "4443"



sem_t sem_server_wait_for_disconnect;
sem_t sem_server_terminado;
sem_t sem_cliente_terminado;
pthread_mutex_t mutex_fp;
bool stop_server = false;


void fp(const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char* str = string_from_vformat(format, arguments);
	va_end(arguments);

	pthread_mutex_lock(&mutex_fp);
	printf("%s", str);
	pthread_mutex_unlock(&mutex_fp);
	free(str);
}



void cliente() {
	fp("cliente >> Conecto el cliente\n");
	int cliente = crear_conexion(SERVER_IP, SERVER_PORT);

	if (cliente >= 0) {
		fp("cliente >> Hola servidor!! te voy a hablar por el socket: %d\n", cliente);
		int r = liberar_conexion(cliente);
		fp("cliente >> resultado de liberar socket cliente: %d\n", r);
	}
	else {
		fp("cliente >> Error de conexion a servidor!! error:%d\n", abs(cliente));
	}

	fp("cliente >> termina hilo\n");
	sem_post(&sem_cliente_terminado);
	pthread_exit(NULL);
}

void listen_server(int *server) {
	char* ip = NULL;

	while (true) {
		if (stop_server)
			break;

		fp("listen_server >> entro en accept\n");
		int c = esperar_cliente(*server, ip);

		if (c < 0) {
			fp("listen_server >> Error de socket: [%d] error:[%s]. cierro el servidor\n", abs(c), strerror(abs(c)));
			break;
		}

		fp("listen_server >> Hola cliente!! tu socket asignado es = %d\n", c);

		if (c > 0) {
			int r = liberar_conexion(c);
			fp("listen_server >> resultado de cerrar el socket cliente:[%d] es %d\n", *server, r);
		}

		fp("listen_server >> Espero por la desconexion del server\n");
		sem_wait(&sem_server_wait_for_disconnect);
		fp("listen_server >> Espera por la desconexion terminada\n");
	}

	fp("listen_server >> termina hilo\n");
	sem_post(&sem_server_terminado);
	pthread_exit(NULL);
}

void test_simple_socket() {
	fp("test_simple_socket >> process id = %d\n", getpid());

	pthread_mutex_init(&mutex_fp, NULL);
	sem_init(&sem_server_terminado, 0, 0);
	sem_init(&sem_cliente_terminado, 0, 0);
	sem_init(&sem_server_wait_for_disconnect, 0, 0);

	int server = iniciar_server(SERVER_IP, SERVER_PORT);

	if (server < 0) {
		fp("test_simple_socket >> error al iniciar el servidor. error= (%d) %s\n", abs(server), strerror(abs(server)));
		sem_destroy(&sem_server_terminado);
		sem_destroy(&sem_cliente_terminado);
		return;
	}

	fp("Server iniciado en socket:[%d]", server);

	pthread_t s_tid;
	pthread_create(&s_tid, NULL, (void*)listen_server, &server);
	pthread_detach(s_tid);

	// le doy tiempo al servidor para que se arme
	usleep(1000000);

	pthread_t c_tid;
	pthread_create(&c_tid, NULL, (void*)cliente, NULL);
	pthread_detach(c_tid);

	sem_wait(&sem_cliente_terminado);

	if (server > 0) {
		stop_server = true;
		int l = liberar_conexion(server);
		fp("test_simple_socket >> resultado de liberar el server = %d :: error = %s\n", l, (l < 0 ? strerror(abs(l)) : ""));
		sem_post(&sem_server_wait_for_disconnect);
	}

	sem_wait(&sem_server_terminado);


	sem_destroy(&sem_server_terminado);
	sem_destroy(&sem_cliente_terminado);

	fp("test_simple_socket >> fin\n");

	pthread_mutex_destroy(&mutex_fp);

	pthread_exit(NULL);
}
