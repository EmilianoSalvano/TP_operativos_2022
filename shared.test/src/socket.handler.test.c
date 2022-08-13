/*
 * socket.handler.test.c
 *
 *  Created on: 22 jun. 2022
 *      Author: utnso
 */

#include "../include/socket.handler.test.h"

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <kiss.global.h>


#include "../../shared/include/test.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/thread_monitor.h"

// Variables de inicializacion
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "4444"
// cantidad maxima de procesos consola creados y ejecutando en simultaneo
#define MAX_CONSOLE_COUNT 3



t_test* test;

// monitor de hilos. Lo uso para detener el cierre del hilo principal
t_thread_monitor* thread_monitor = NULL;

// este flag me sirve para indicarle al productor de consolas que se detenga
bool terminate_cosole_producer = false;

sem_t sem_console_counter;
//sem_t sem_server_ready;
//sem_t sem_console_ready;


static t_handshake_result server_handshake(t_socket_handler* server);
static void server_disconnected(t_socket_handler* server);
static void server_listening(t_socket_handler* server);
static void server_error(t_socket_handler* server);
static void handle_client(t_socket_handler* client);
static void console_producer();
static void handle_console();
static t_handshake_result handshake_console(t_socket_handler* console);



/*
 * @OBJETIVO DEL TEST
 *
 * emular el paso de mensajes entre un servidor y consolas
 * 1. Las consolas se crean en inmediatamente piden conexion al servidor
 * 2. El servidor controla que el cliente que esta pidiendo la conexion sea un proceso de tipo consola. Esto lo hace durante el handshake
 * 3. Si el cliente es aceptado, envia un pcb al servidor
 * 4. el servidor deserializa el pcb y lo imprime por pantalla
 *
 * El test utiliza la estructura la biblioteca de manejadores de sockets que son u grupo de funciones que facilitan la interfaz para la creacion
 * de servidores, clientes y el paso de mensajes. Ademas cuanta con la estructura t_socket_handler que guarda informacion de la conexion abierta del socket
 *
 * El test tambien utiliza la biblioteca de monitor de hilos. El monitor funciona a traves de la estructura t_thread_monitor la cual
 * se utiliza para monitorear el estado de los hilos y permite saber si existen hilos en ejecucion en el proceso
 */

void test_socket_handler() {
	test = test_open_async("test.socket.handler");

	//sem_init(&sem_server_ready, 0, 0);
	//sem_init(&sem_console_ready, 0, 0);

	thread_monitor = thread_monitor_create();

	// creo al seridor que va a estar escuchando la conexion de los procesos externos
	t_socket_handler* server = start_server(SERVER_IP, SERVER_PORT);

	// siempre hay que controlar si hubo un error en el socket
	if (server->error != 0) {
		test_fprint(test, "Ocurrio un error iniciando el servidor. Error:[%d] >> %s || %s (%d)", server->error, strerror(server->error), __FILE__, __LINE__);
		goto error_start_server;
	}

	test_fprint(test, "test_socket_handler >> server conectado->socket:[%d] || %s (%d)\n", server->socket, __FILE__, __LINE__);

	// asigno el metodo que quiero que el servidor use para hacer el handshake con la conexion de un nuevo proceso
//server->handle_handshake = server_handshake;
	// asigno el metodo que va a invocar el servidor cada vez que una conexion de un nuevo proceso sea aceptada
	server->on_client_connected = handle_client;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	server->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	server->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	server->on_error = server_error;
	// asigno el monitor de hilos para que vaya monitoreando todos los hilos que se crean dentro del entorno de la biblioteca de sockets
	server->thread_monitor = thread_monitor;

	// pongo a escuchar al servidor de forma asincrona, en un hilo independiente, para que no me bloquee el hilo principal
	server_listen_async(server);

	// espero a que el server este listo y ecuchando, asi las consolas se pueden conectar
	//sem_wait(&sem_server_ready);

	// activo el prductor de consolas
	pthread_t tid;
	if (pthread_create(&tid, NULL, (void*)console_producer, NULL) != 0) {
		test_fprint(test, "Ocurrio un error iniciando el hilo del productor de consolas. Error:[%d] >> %s || %s (%d)", errno, strerror(errno), __FILE__, __LINE__);
		goto error_iniciando_productor;
	}

	pthread_detach(tid);

	// espero la señal del usuario para cerrar el sistema
	test_fprint(test, "Presione cualquier tecla para terminar el proceso ...\n");
	fgetc(stdin);

	//test_fprint(test, "test_socket_handler >> espero por el servidor y el productor de consolas || %s (%d)\n", __FILE__, __LINE__);

	// sem_wait(&sem_console_ready);
	// sem_destroy(&sem_console_ready);

	//test_fprint(test, "test_socket_handler >> salgo del bloqueo y cierro el proceso || %s (%d)\n", __FILE__, __LINE__);


	// detengo el productor de consolas
	terminate_cosole_producer = true;

	// desconecto el servidor, asi sale del hilo
	test_fprint(test, "test_socket_handler >> disconnect server->socket:[%d] || %s (%d)\n", server->socket, __FILE__, __LINE__);
	socket_handler_disconnect(server);


	//thread_monitor->wait_for_terminated(thread_monitor);
	//test_fprint(test, "test_socket_handler >> bloqueo hilo principal con monitor. destruye server || %s (%d)\n", __FILE__, __LINE__);

	test_fprint(test, "test_socket_handler >> Aguardo por finalizacion de hilos (Monitor) ... || %s (%d)\n", __FILE__, __LINE__);
	// bloqueo el hilo principal hasta que el monitor de hilos que me indique si todos los hilos secundarios terminaron de ejecutarse
	thread_monitor->terminate_and_wait(thread_monitor);

	test_fprint(test, "test_socket_handler >> Monitor desbloqueado. Continua hilo principal || %s (%d)\n", __FILE__, __LINE__);

	test_fprint(test, "test_socket_handler >> Destruyo server || %s (%d)\n", __FILE__, __LINE__);
	socket_handler_destroy(server);

	test_fprint(test, "test_socket_handler >> Destruyo monitor || %s (%d)\n", __FILE__, __LINE__);
	thread_monitor_destroy(thread_monitor);

	goto finally;

error_iniciando_productor:
error_start_server:
	socket_handler_destroy(server);
	goto finally;

finally:

	// Para Valgrind. Intento que los hilos se cierren antes del hilo principal. Si eso no pasa, Valgrind pierde la referencia y lo muestra como un memory leak
	// que en realidad no es asi
	usleep(2000000);

	//sem_destroy(&sem_server_ready);
	test_close(test);
	pthread_exit(NULL);
}



/********************************************************************************************
* SERVIDOR:
* escucha las solicitudes de conexion de las consolas
* Por cada consola conectada, crea un cliente para atenderla que se ejecuta sobre un hilo independiente
********************************************************************************************/

// el metodo es invocado por el manejador del socket cuando el servidor esta listo para recibir conexiones
static void server_listening(t_socket_handler* server) {
	test_fprint(test, "server_ready >> Servidor preparado y escuchando ... || %s (%d)\n", __FILE__, __LINE__);
	//sem_post(&sem_server_ready);
}

// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	test_fprint(test, "server_close >> El servidor ha terminado su ejecucion || %s (%d)\n", __FILE__, __LINE__);
}

// el metodo es invocado por el manejador de socket cuando ocurre algun error con el servidor durante el "accept"
// TODO: el handler deberia ser del nuevo socket, no del server
static void server_error(t_socket_handler* server) {
	test_fprint(test, "server_error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: Error:[%d] %s  || %s (%d)\n",
			server->error, strerror(server->error), __FILE__, __LINE__);
}

/*
 * el metodo implementa el handshake que hace el servidor con un proceso que pide conexion
 */
// TODO: completar el manejo de errores del handshake-servidor
static t_handshake_result server_handshake(t_socket_handler* server) {

	// en el handshake solicito el codigo de proceso. Si no es consola, lo rechazo
	t_package* package = recieve_message(server);

	if (package->header != MC_PROCESS_CODE) {
		test_fprint(test, "server_handshake >> Server:[%d] -> Codigo de header:[%d] desconocido para operacion de handshake || %s (%d)\n",
				server->socket, package->header, __FILE__, __LINE__);
		return CONNECTION_REJECTED;
	}

	t_process_code process_code = deserialize_process_code(package->payload);

	buffer_destroy(package->payload);
	package_destroy(package);

	if(process_code != PROCESS_CONSOLE) {
		test_fprint(test, "server_handshake >> Server:[%d] -> Codigo de proceso:[%d] incorrecto para operacion de handshake || %s (%d)",
				server->socket, process_code, __FILE__, __LINE__);
		return CONNECTION_REJECTED;
	}

	send_signal(server, MC_SIGNAL, SG_CONNECTION_ACCEPTED);

	return CONNECTION_ACCEPTED;
}



/********************************************************************************************
* CLIENTE:
* Los clientes son las conexiones establecidas de los procesos pero del lado del servidor
********************************************************************************************/

/*
* Este metodo se ejecuta en un hilo secundario y es invocado cada vez que se crea una nueva
* conexion de un proceso al servidor
* Utiliza un hilo secundario porque el servidor esta configurado para trabajar de forma asincrona
*/
static void handle_client(t_socket_handler* client) {
	t_pcb* pcb;
	char* s;

	test_fprint(test, "handle_client >> Cliente:[%d] -> nuevo cliente conectado || %s (%d)\n", client->socket, __FILE__, __LINE__);

	test_fprint(test, "handle_client >> Cliente:[%d] -> Esperando mensaje de consola || %s (%d)\n", client->socket, __FILE__, __LINE__);
	t_package* package = recieve_message(client);

	if (client->error != 0) {
		test_fprint(test, "handle_client >> Cliente:[%d] -> Ocurrio un error al recibir el paquete. Error:[%d] %s || %s (%d)\n",
				client->error, strerror(client->error), client->socket, __FILE__, __LINE__);

		// TODO: analizar que tipo de error es, para saber si el socket sigue vivo.
		// puedo seguir y que quiza falle tambien en el send. no pasa nada
		goto error_paquete;
	}

	switch (package->header) {
	case MC_PCB:
		pcb = deserializar_pcb(package->payload);
		s = pcb_to_string(pcb);
		test_fprint(test, "handle_client >> Cliente:[%d] -> PCB:[%d] recibido :: %s || %s (%d)\n", client->socket, pcb->id, s, __FILE__, __LINE__);

		pcb_destroy(pcb);
		free(s);
		break;
	default:
		test_fprint(test, "handle_client >> Cliente:[%d] -> mensaje recibido con header:[%d] incorrecto || %s (%d)\n", client->socket, package->header, __FILE__, __LINE__);
	}


error_paquete:

	if (package != NULL && package->payload != NULL)
		buffer_destroy(package->payload);

	if (package != NULL)
		package_destroy(package);

	test_fprint(test, "handle_client >> Cliente:[%d] -> envio señal 'MC_CLOSING_CONNECTION' a consola || %s (%d)\n", client->socket, __FILE__, __LINE__);
	send_signal(client, MC_SIGNAL, SG_CLOSING_CONNECTION);

	if (client->error != 0) {
		test_fprint(test, "handle_client >> Cliente:[%d] -> Ocurrio un error al enviar el paquete. Error:[%d] %s || %s (%d)\n",
						client->error, strerror(client->error), client->socket, __FILE__, __LINE__);
	}

	test_fprint(test, "handle_client >> Cliente:[%d] -> operacion completada! || %s (%d)\n", client->socket, __FILE__, __LINE__);


error_conexion:

	test_fprint(test, "handle_client >> disconnect cliente->socket:[%d] || %s (%d)\n", client->socket, __FILE__, __LINE__);
	socket_handler_disconnect(client);

	if (client->error != 0) {
		test_fprint(test, "handle_client >> Cliente:[%d] -> Ocurrio un error al liberar el socket. Error:[%d] %s || %s (%d)\n",
						client->error, strerror(client->error), client->socket, __FILE__, __LINE__);
	}

	//socket_handler_destroy(client);
}


/********************************************************************************************
* CONSOLA:
* las consolas son los procesos externos que soicitan conexion al servidor
* Se intenta emular un proceso independiente
********************************************************************************************/

/*
 * el metodo crea procesos de forma aleatoria
 */
static void console_producer() {

	pthread_t self_tid = pthread_self();
	test_fprint(test, "console_producer >> Inicio el productor de consolas || %s (%d)\n", __FILE__, __LINE__);
	// agrego el hilo al monitor
	thread_monitor->subscribe(thread_monitor, self_tid);


	//sem_init(&sem_console_counter, 0, MAX_CONSOLE_COUNT);
	srand(time(0));
	int t;
	while(true) {
		//sem_wait(&sem_console_counter);

		t = rand() % 20;
		if (t == 0) t = 1;
		test_fprint(test, "console_producer >> duermo (%.2f) seg || %s (%d)\n", (t*1.0)/10, __FILE__, __LINE__);
		usleep(t * 100000);

		if (terminate_cosole_producer)
			break;

		pthread_t tid;
		if (pthread_create(&tid, NULL, (void*)handle_console, NULL) != 0) {
			test_fprint(test, "console_producer >> Error creando la consola. Error:[%d] %s || %s (%d)\n", errno, strerror(errno), __FILE__, __LINE__);
			continue;
		}

		pthread_detach(tid);

		// TODO: quitar luego del debug
		break;
	}

	//sem_destroy(&sem_console_counter);
	// quito el hilo del monitor
	thread_monitor->unsubscribe(thread_monitor, self_tid);

	test_fprint(test, "console_producer >> Fin de ejecucion del productor de consolas || %s (%d)\n", __FILE__, __LINE__);
}

// TODO: completar el manejo de errores del handshake-consola
static t_handshake_result handshake_console(t_socket_handler* console) {

	test_fprint(test, "handshake_console >> Consola:[%d] solicitud de conexion de nuevo cliente. Procesando handshake...\n", console->socket);
	//envio el tipo de proceso
	send_signal(console, MC_PROCESS_CODE, PROCESS_CONSOLE);

	// aguardo la respuesta del server (aceptado o rechazado)
	t_package* package = recieve_message(console);

	t_handshake_result result;

	if (package == NULL) {
		test_fprint(test, "handshake_console >> Consola:[%d] Error recibiendo mensaje del cliente. handshake abortado\n", console->socket, package->header);
		return CONNECTION_REJECTED;
	}

	if (package->header != MC_SIGNAL) {
		test_fprint(test, "handshake_console >> Consola:[%d] Mensaje con header:[%d] incorrecto para handshake\n", console->socket, package->header);
		result = CONNECTION_REJECTED;
	}
	else {
		t_handshake_result status = (t_handshake_result)deserialize_signal(package->payload);

		if (status == CONNECTION_ACCEPTED) {
			test_fprint(test, "handshake_console >> Consola:[%d] conexion aceptada por el servidor\n", console->socket);
			result =  CONNECTION_ACCEPTED;
		}
		else {
			test_fprint(test, "handshake_console >> Consola:[%d] conexion rechazada por el servidor\n", console->socket);
			result = CONNECTION_REJECTED;
		}
	}

	buffer_destroy(package->payload);
	package_destroy(package);

	return result;
}


static void handle_console() {
	pthread_t tid = pthread_self();
	test_fprint(test, "handle_console >> Inicio de envio de datos || %s (%d)\n", __FILE__, __LINE__);

	thread_monitor->subscribe(thread_monitor, tid);

	// creo el proceso y le paso la ip y puerto del server
	t_socket_handler *console = connect_to_server(SERVER_IP, SERVER_PORT);

	if (console->error != 0 || !console->connected) {
		test_fprint(test, "handle_console >> Consola:[%d] error de conexion al servidor. Consola terminada || %s (%d)\n", console->socket, __FILE__, __LINE__);
		goto finally;
	}

	/*
	// handshake
	if (handshake_console(console) != CN_ACEPTADA)
		goto finally;
*/

	// doy aviso que la consola esta creada y conectada con el cliente
	//sem_post(&sem_console_ready);

	test_fprint(test, "handle_console >> Consola:[%d] conexion aceptada por el servidor. Enviando mensaje... || %s (%d)\n", console->socket, __FILE__, __LINE__);

	// envio el mensaje
	t_pcb* pcb = produce_pcb();
	t_buffer* buffer = serializar_pcb(pcb);
	char* s_pcb = pcb_to_string(pcb);

	// no hace falta usar el metodo asincrono. Ya estoy en un hilo distinto al proncipal
	send_message(console, MC_PCB, buffer);

	if (console->error != 0) {
		test_fprint(test, "handle_console >> Consola:[%d] error enviando el mensaje. Error[%d] %s || %s (%d)\n",
				console->socket, console->error, strerror(console->error), __FILE__, __LINE__);
	}
	else {
		test_fprint(test, "handle_console >> Consola:[%d] PCB:[%d] enviado :: %s || %s (%d)\n", console->socket, pcb->id, s_pcb, __FILE__, __LINE__);
	}

	free(s_pcb);
	pcb_destroy(pcb);
	buffer_destroy(buffer);

	// me quedo esperando que el servidor procese y me envie la respuesta de finalizacion con el resultado del proceso
	test_fprint(test, "handle_console >> Consola:[%d] esperando respuesta del servidor ... || %s (%d)\n", console->socket, __FILE__, __LINE__);
	t_package* package = recieve_message(console);

	if (console->error != 0) {
		test_fprint(test, "handle_console >> Consola:[%d] error recibiendo el mensaje. Error[%d] %s || %s (%d)\n",
						console->socket, console->error, strerror(console->error), __FILE__, __LINE__);
	}
	else if (package != NULL && package->header == SG_CLOSING_CONNECTION) {
		test_fprint(test, "handle_console >> Consola:[%d] imprime stats ... || %s (%d)\n", console->socket, __FILE__, __LINE__);
	}
	else {
		test_fprint(test, "handle_console >> Consola:[%d] ocurrio un error al recibir el mensaje del servidor || %s (%d)\n", console->socket, __FILE__, __LINE__);
	}

	// libero la memoria
	if (package != NULL) {
		if (package->payload != NULL)
			buffer_destroy(package->payload);

		package_destroy(package);
	}

finally:

	test_fprint(test, "handle_console >> disconnect->socket:[%d] || %s (%d)\n", console->socket, __FILE__, __LINE__);

	socket_handler_disconnect(console);

	if (console->error != 0) {
		test_fprint(test, "handle_console >> Consola:[%d] error desconectando socket. Error[%d] %s || %s (%d)\n",
								console->socket, console->error, strerror(console->error), __FILE__, __LINE__);
	}

	socket_handler_destroy(console);

	// TODO: descomentar
	//sem_post(&sem_console_counter);

	thread_monitor->unsubscribe(thread_monitor, tid);
	test_fprint(test, "handle_console >> consola terminada || %s (%d)\n", __FILE__, __LINE__);

	pthread_exit(NULL);
}





