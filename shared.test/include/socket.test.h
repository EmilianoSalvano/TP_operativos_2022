/*
 * socket.test.h
 *
 *  Created on: 5 jun. 2022
 *      Author: utnso
 */

#ifndef SOCKET_TEST_H_
#define SOCKET_TEST_H_

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "../../shared/include/kiss.global.h"
#include "../../shared/include/conexiones.h"
#include "../../shared/include/test.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"


#define CLIENTS_CAPACITY 100
#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_IP_PORT "4444"

// defino el enumerador otra vez aca para porque este test no utiliza la biblioteca "socket_handler.h"
// pero de todos modos quiero usar los codigos
typedef enum {
	CON_RECHAZADA = -1,
	CON_ACEPTADA = 1
} t_conexion;



// estructuras

// esta estructura la creo para poder utilizar nombres de variables globales libremenmte entre todos los test
// por ejemplo "test" va a estar repetido en todos pero si va dentro de la estructura la convierto en variable local a la estructura
// y la estructura seria la global. La estructura deberia tener el nombre del test que estoy llevando a cabo
typedef struct {
	t_test* 		test;
	pthread_mutex_t mutex_print_msg;
	sem_t			sem_server_iniciado;
	sem_t			sem_consola_terminada;
	sem_t			sem_cliente_terminado;
	int				cantidad_envios;
} t_socket_test;


typedef struct {
	int		socket;
	char*	port;
	int		clients_count;
	int		clients[CLIENTS_CAPACITY];		// para simplificar le asigno una tamaño
} t_server_handler;

typedef struct {
	int			socket;
	char*		server_ip;
	char*		server_port;
} t_console_handler;



// Variable global para contener a otras variables globales. Se usa para que otros .h no entren en conflicto
t_socket_test global_socket_test;


/*
 * @NAME: test_socket
 * @DESC:
 *
 */
void test_socket();


/*
 * @NAME: test_server_handler
 * @DESC: hilo de atencion de conexiones de consola
 *
 */
void test_server_handler(t_server_handler* handler);


/*
 * @NAME: test_client_handler
 * @DESC: funcion que funciona en un hilo y representa la conexion de una consola al servidor
 *
 */
void test_client_handler(int *socket);


/*
 * @NAME: test_client_console
 * @DESC: funcion que se ejecuta en un hilo y simula un proceso consola que envia un mensaje al servidor
 *
 */
void test_console_handler(t_console_handler* handler);


/*
 * @NAME: test_console_send
 * @DESC: envia un stream por el socket pasado por parametro
 *
 */
void test_console_send_pcb();


void test_print_message(t_package* package);
void test_print_pcb_message(t_buffer* buffer);
void test_print_instrucciones_message(t_buffer* buffer);



#endif /* SOCKET_TEST_H_ */
