#include "../include/socket.test.h"


/*
 * socket.test.c
 *
 *  Created on: 5 jun. 2022
 *      Author: utnso
 */

#include <sys/socket.h>


/*
 * Este metodo ejecuta recv que es una funcion bloqueante, por lo tanto, el thread entero se va a bloquear hasta que le llegue algo al socket
 * En este ejecmplo no hice nada todavia para evitar esto, pero en el TP no es la idea dejar _todo bloqueado cuando se cierra el programa
 * Los hilos tienen que ser terminados, los sockets liberados y tambien la memoria utilizada
 *
 */
void test_server_handler(t_server_handler* handler) {
	pthread_t 		client_thread;
	int* 			client_socket;
	uint32_t 		handshake;
	uint32_t 		handshake_response;
	char* 			client_ip = NULL;

	handler->socket = iniciar_server(SERVER_IP_ADDRESS, handler->port);

	if (handler->socket < 0) {
		printf("No se pudo iniciar el servidor. handler->socket: %d\n", handler->socket);
		return;
	}

	sem_post(&(global_socket_test.sem_server_iniciado));

	while (true) {
		client_socket = malloc(sizeof(int));

		// la funcion ejecuta una syscall bloqueante. Se destraba cuando un cliente establece conexion
		*client_socket = esperar_cliente(handler->socket, client_ip);

		printf("Cliente ip:[%s] conectado\n", client_ip);

		free(client_ip);

		// en el handshake solicito el codigo de proceso. Si no es consola, lo rechazo
		recv(*client_socket, &handshake, sizeof(uint32_t), MSG_WAITALL);

		if(handshake == PROCESS_CONSOLE && handler->clients_count < CLIENTS_CAPACITY)
			handshake_response = CON_ACEPTADA;
		else {
			handshake_response = CON_RECHAZADA;
		}

		send(*client_socket, &handshake_response, sizeof(uint32_t), 0);

		if (handshake_response == CON_ACEPTADA) {
			handler->clients[handler->clients_count] = *client_socket;		// pincha y no se por que
			handler->clients_count++;

			// creo el hilo que va a atender la nueva conexion y le paso el socket
			pthread_create(&client_thread, NULL, (void*)test_client_handler, client_socket);
			pthread_detach(client_thread);
		}

		// para el ejemplo solo acepto una conexion de consola
		break;
	}

	liberar_conexion(handler->socket);
}


void test_client_handler(int *client_socket) {
	uint32_t	header;
	t_package* 	package;
	int			recibidos = 0;

	while(true) {
		// header del mensaje
		recv(*client_socket, &header, sizeof(uint32_t), MSG_WAITALL);

		// podria haber una logica aca para validar el header del mansaje.
		// hay que ver como hacer para descartar el mensaje si no es valido

		package = malloc(sizeof(t_package));
		package->payload = malloc(sizeof(t_buffer));
		package->header = header;

		// tamaño del mensaje
		recv(*client_socket, &(package->payload->size), sizeof(uint32_t), MSG_WAITALL);

		// ahora que tengo la longitud total del mensaje. Lo pido entero
		package->payload->stream = malloc(package->payload->size);
		recv(*client_socket, package->payload->stream, package->payload->size, MSG_WAITALL);

		test_print_message(package);

		// truchada para que corte la ejecucion. Solo hago la cantidad de envios definidos en configuracion
		recibidos++;
		if (recibidos >= global_socket_test.cantidad_envios)
			break;
	}

	// la memoria del paquete se va liberando mientras desarmo el mensaje
	liberar_conexion(*client_socket);
	free(client_socket);

	buffer_destroy(package->payload);
	package_destroy(package);

	sem_post(&(global_socket_test.sem_cliente_terminado));
}


void test_print_message(t_package* package) {
	// el mutex lo uso para que imprima _todo el mensaje de corrido y no quede intercalado con otro mensaje
	pthread_mutex_lock(&(global_socket_test.mutex_print_msg));

	switch(package->header) {
		case MC_PCB:
			test_print_pcb_message(package->payload);
			break;
		/*
		case MC_INSTRUCTION_LIST:
			test_print_instrucciones_message(package->payload);
			break;
			*/
		default:
			printf("mensaje no identificado con id: %d", package->header);
	}

	pthread_mutex_unlock(&(global_socket_test.mutex_print_msg));
}


void test_print_pcb_message(t_buffer* buffer) {
	t_pcb* pcb = deserializar_pcb(buffer);

	test_print_separator(global_socket_test.test, "PCB recibido por socket");
	char* string_pcb = pcb_to_string(pcb);
	printf("%s\n", string_pcb);

	free(string_pcb);
	pcb_destroy(pcb);
}


void test_print_instrucciones_message(t_buffer* buffer) {
	t_list* list = deserializar_lista_instrucciones(buffer);

	test_print_separator(global_socket_test.test, "Instrucciones recibidas de la consola");
	char* string_list = instruccion_list_to_string(list);
	printf("%s\n", string_list);

	free(string_list);
	list_destroy_and_destroy_elements(list, instruccion_destroy_in_list);
}


void test_console_handler(t_console_handler* handler) {
	// el semaforo impide que primero se inicialice la consola e intente conectarse a una servidor
	// que todavia no esta listo
	sem_wait(&(global_socket_test.sem_server_iniciado));

	handler->socket = -1;
	// me conecto al servidor y realizo un pequeño handshake
	handler->socket = crear_conexion(handler->server_ip, handler->server_port);

	if (handler->socket < 0) {
		printf("Error de conexion de consola. handler->socket=%d\n", handler->socket);
		goto finally;
	}

	// me identifico como proceso consola
	uint32_t request_handshake = PROCESS_CONSOLE;
	uint32_t response_handshake;

	send(handler->socket, &request_handshake, sizeof(uint32_t), 0);
	recv(handler->socket, &response_handshake, sizeof(uint32_t), MSG_WAITALL);

	if (response_handshake != CON_ACEPTADA) {
		printf("conexion rechazada por el servidor\n");
		goto finally;
	}

	test_console_send_pcb(handler->socket);
	usleep(1000000);
	test_console_send_pcb(handler->socket);

	// impide que se cierre hasta que el cliente termine.
	// Es para no perder la conexion del socket y darle tiempo al cliente para que lea el buffer
	sem_wait(&(global_socket_test.sem_cliente_terminado));

finally:

	if (handler->socket > 0)
		liberar_conexion(handler->socket);

	sem_post(&(global_socket_test.sem_consola_terminada));
}


void test_console_send_pcb(int socket) {
	t_package* package = malloc(sizeof(t_package));
	package->header = MC_PCB;

	t_pcb* pcb = produce_pcb();
	test_print_separator(global_socket_test.test, "PCB generado en consola");

	char* pcb_string = pcb_to_string(pcb);
	printf("%s\n", pcb_string);

	package->payload = serializar_pcb(pcb);

	uint32_t package_size;
	void* stream = serializar_paquete(package, &package_size);

	// enviar mensaje
	send(socket, stream, package_size, 0);

	buffer_destroy(package->payload);
	package_destroy(package);
	free(stream);
	free(pcb_string);
}

/*
 * Los hilos que manejan conexiones se bloquean cuando entran en las funciones "recv" o "accept"
 * Hay que encontrar la forma de porder cerrar el proceso y que todos los hilos terminen su ejecucion y liberen recursos
 * Algunas soluciones pueden ser:
 *
 * 1. hacer que el recv no bloquee y usar un while con un sleep para preguntar todo el tiempo. Eso deja meter un flag en el medio para indicar
 * debe detener su ejecucion pero tambien genera una espera activa y eso estra prohibido en el TP
 * 2. usar select. es mas o menos lo mismo que arriba. tampoco se puede usar
 * 3. matar el hilo. Pero luego hay que ver como liberamos los sockets. quiza sea la mejor opcion. despues lo investigo
 *
 */

void test_socket() {
	pthread_t server_thread;
	pthread_t console_thread;

	global_socket_test.test = test_open("test_socket");
	pthread_mutex_init(&(global_socket_test.mutex_print_msg), NULL);

	sem_init(&(global_socket_test.sem_server_iniciado), 0, 0);
	sem_init(&(global_socket_test.sem_consola_terminada), 0, 0);
	sem_init(&(global_socket_test.sem_cliente_terminado), 0, 0);

	global_socket_test.cantidad_envios = 2;

	t_server_handler* server_handler = malloc(sizeof(t_server_handler));
	server_handler->port = SERVER_IP_PORT;
	server_handler->socket = -1;
	server_handler->clients_count = 0;
	for (int i = 0; i < CLIENTS_CAPACITY; i++) {
		server_handler->clients[i] = -1;
	}

	t_console_handler* console_handler = malloc(sizeof(t_console_handler));
	console_handler->server_ip = strdup(SERVER_IP_ADDRESS);
	console_handler->server_port = SERVER_IP_PORT;
	console_handler->socket = -1;

	// hilo para servidor. Acepta conexiones de consola
	pthread_create(&server_thread, NULL, (void*)test_server_handler, server_handler);
	pthread_detach(server_thread);

	// hilo para emular un proceso consola
	pthread_create(&console_thread, NULL, (void*)test_console_handler, console_handler);
	pthread_detach(console_thread);

	sem_wait(&(global_socket_test.sem_consola_terminada));

	test_close(global_socket_test.test);
	pthread_mutex_destroy(&(global_socket_test.mutex_print_msg));

	free(server_handler);
	free(console_handler);
}
