/*
 * socket_handlers.c
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>


#include <commons/collections/queue.h>

#include "../include/kiss.error.h"
#include "../include/socket_handler.h"
#include "../include/conexiones.h"


typedef struct {
	t_socket_handler*	handler;
	uint32_t 			header;
	t_buffer*			buffer;
	void				(*callback)(struct t_socket_handler*);
} t_socket_thread_callback;



/*
 * *********************************** declaracion de metodos privados ********************************
 */
static void reset_status(t_socket_handler* handler);
static void thread_recieve_message_async(t_socket_thread_callback* thread_callbak);
static void thread_send_message_async(t_socket_thread_callback* thread_callbak);
static void thread_handle_client(t_socket_handler* client);
static void handle_client(t_socket_handler *client);
static void handle_server(t_socket_handler *server);
static t_socket_handler* socket_handler_create(t_socket_type type, const char* ip, const char* port);
static void socket_handler_destroy_in_list(void* handler);
static t_socket_thread_callback* thread_callback_create(t_socket_handler* handler, uint32_t header, t_buffer* buffer, void (*callback)(struct t_socket_handler*));
static void thread_callback_destroy(t_socket_thread_callback* self);
static int thread_create(t_thread_monitor* monitor, pthread_t* tid, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);


/*
 * ************************************ Implementacion *************************************
 */

t_socket_handler* server_handler_create(const char* ip, const char* port){
	t_socket_handler* self = socket_handler_create(SOCKET_TYPE_SERVER, ip, port);

	self->clients = list_create();
	return self;
}


t_socket_handler* client_handler_create(const char* ip, const char* port){
	return socket_handler_create(SOCKET_TYPE_CLIENT, ip, port);
}


int socket_handler_destroy(t_socket_handler* self) {
	if (self->connected)
		return EXIT_FAILURE;

	queue_destroy(self->data);

	if (self->name != NULL)
		free(self->name);

	if (self->ip != NULL)
		free(self->ip);

	if (self->port != NULL)
		free(self->port);

	if (self->type == SOCKET_TYPE_SERVER && self->clients != NULL)
		list_destroy(self->clients);

	free(self);
	return EXIT_SUCCESS;
}



int socket_handler_disconnect(t_socket_handler* self) {
	if (!self->connected)
		return 0;

	if (self->type == SOCKET_TYPE_SERVER)
		self->disconnecting = true;

	self->reset(self);
	self->connected = false;

	int r = liberar_conexion(self->socket);

	if (r < 0) {
		self->error = abs(r);
		self->last_return = -1;
		self->connected = true;
	}

	return r;
}


t_socket_handler* start_server(const char* ip, const char* port) {

	t_socket_handler *server = server_handler_create(ip, port);
	server->last_return = iniciar_server(server->ip, server->port);

	if (server->last_return >= 0) {
		server->socket = server->last_return;
		server->connected = true;
	}
	else {
		server->error = abs(server->last_return);
	}

	return server;
}


void server_listen(t_socket_handler* server) {
	handle_server(server);
}


pthread_t server_listen_async(t_socket_handler* server) {
	server->async = true;

	pthread_t tid;
	int r = pthread_create(&tid, NULL, (void*)handle_server, server);

	if (r != 0) {
		server->error = r;
		return -1;
	}

	pthread_detach(tid);
	return tid;
}


t_socket_handler* connect_to_server(const char* ip, const char* port) {
	t_socket_handler *client = client_handler_create(ip, port);
	client->last_return = crear_conexion(client->ip, client->port);

	if (client->last_return >= 0) {
		client->socket = client->last_return;
		client->connected = true;
	}
	else {
		client->error = abs(client->last_return);
	}

	return client;
}



t_package* recieve_message(t_socket_handler* handler) {
	int err;
	int header;
	t_package* package = package_create();
	package->payload = buffer_create();
	handler->reset(handler);

	// cuando recv devuelve 0, es porque se corto la conexion. Por eso evaluo por <=0

	// header del mensaje
	handler->last_return = recv(handler->socket, &(package->header), sizeof(uint32_t), MSG_WAITALL);
	err = errno;

	if (handler->last_return == 0) {
		handler->error = SKT_CONNECTION_LOST;
		package->header = EMPTY_PACKAGE;
		//goto error;
	}
	else if (handler->last_return < 0) {
		handler->error = err;
		package->header = EMPTY_PACKAGE;
		//goto error;
	}
	else {
		// tamaño del mensaje
		handler->last_return = recv(handler->socket, &(package->payload->size), sizeof(uint32_t), MSG_WAITALL);
		err = errno;

		if (handler->last_return == 0) {
			handler->error = SKT_CONNECTION_LOST;
			package->header = ERROR_PACKAGE;
			//goto error;
		}
		else if (handler->last_return < 0) {
			handler->error = err;
			package->header = ERROR_PACKAGE;
			//goto error;
		}
		else if (package->payload->size > 0) {

			package->payload->stream = malloc(package->payload->size);
			handler->last_return = recv(handler->socket, package->payload->stream, package->payload->size, MSG_WAITALL);
			err = errno;

			if (handler->last_return == 0) {
				handler->error = SKT_CONNECTION_LOST;
				package->header = ERROR_PACKAGE;
				//goto error;
			}
			else if (handler->last_return < 0) {
				handler->error = err;
				package->header = ERROR_PACKAGE;
				//goto error;
			}
			else if (package->payload->size != handler->last_return) {
				handler->error = SKT_DATA_TRANF_INCOMPLETE;
				package->header = ERROR_PACKAGE;
			}
		}
	}

	return package;

/*
error:
	buffer_destroy(package->payload);
	package_destroy(package);
	return NULL;
*/
}


// TODO: pasar el callback en una estructura de parametros. Crear el paquete y buffer en este metodo y dejarlo en data para que el que llama la funcion pueda liberar la memoria
pthread_t recieve_message_async(t_socket_handler* handler, void(*callback)(t_socket_handler*)) {
	pthread_t tid;
	t_socket_thread_callback* thread_callback = thread_callback_create(handler, 0, NULL, callback);

	int r = pthread_create(&tid, NULL, (void*)thread_recieve_message_async, thread_callback);

	if (r != 0) {
		handler->error = r;
		thread_callback_destroy(thread_callback);

		if (handler->on_error != NULL)
			handler->on_error(handler);

		return -1;
	}

	pthread_detach(tid);
	return tid;
}


int send_signal(t_socket_handler* handler, uint32_t header, uint32_t signal) {
	t_buffer* buffer = serialize_uint32t(signal);
	int result = send_message(handler, header, buffer);
	buffer_destroy(buffer);

	return result;
}


pthread_t send_signal_async(t_socket_handler* handler, uint32_t header, uint32_t signal, void(*callback)(t_socket_handler*)) {
	t_buffer* buffer = serialize_uint32t(signal);
	return send_message_async(handler, header, buffer, callback);
}


int send_message(t_socket_handler* handler, uint32_t header, t_buffer* buffer) {
	bool _destroy_buffer = false;
	t_package* package = package_create();
	package->header = header;
	handler->reset(handler);
	package->payload = buffer;

	if (buffer == NULL) {
		package->payload = buffer_create();
		package->payload->size = 0;
		package->payload->stream = NULL;
		_destroy_buffer = true;
	}
	else
	{
		package->payload = buffer;
	}

	uint32_t package_size;
	void* stream = serializar_paquete(package, &package_size);
	handler->last_return = send(handler->socket, stream, package_size, 0);

	if (handler->last_return <= 0) {
		handler->error = errno;

		if (handler->error == 0)
			handler->error = SKT_DATA_TRANF_INCOMPLETE;
	}
	else if (handler->last_return < package_size) {
		handler->error = SKT_DATA_TRANF_INCOMPLETE;
	}

	if (_destroy_buffer)
		buffer_destroy(package->payload);

	package_destroy(package);
	free(stream);

	return handler->last_return;
}


pthread_t send_message_async(t_socket_handler *handler, uint32_t header, t_buffer* buffer, void(*callback)(t_socket_handler*)) {
	t_socket_thread_callback* thread_callback =  thread_callback_create(handler, header, buffer, callback);

	pthread_t tid;
	int r = pthread_create(&tid, NULL, (void*)thread_send_message_async, thread_callback);

	if (r != 0) {
		handler->error = r;
		thread_callback_destroy(thread_callback);

		if (handler->on_error != NULL)
			handler->on_error(handler);

		return -1;
	}

	pthread_detach(tid);
	return tid;
}



/***************** Private ******************************/

static void thread_recieve_message_async(t_socket_thread_callback* thread_callback) {
	pthread_t tid;
	t_socket_handler* handler = thread_callback->handler;

	if (handler->thread_monitor != NULL) {
		tid = pthread_self();
		handler->thread_monitor->subscribe(handler->thread_monitor, tid);
	}

	t_package* package = recieve_message(handler);
	queue_push(handler->data, package);
	//socket_handler_data_add(handler, (void*)package);

	thread_callback->callback(handler);
	thread_callback_destroy(thread_callback);

	// si una monitor de hilos, entonces le aviso que este hilo completo su ejecucion
	if (handler->thread_monitor != NULL) {
		handler->thread_monitor->unsubscribe(handler->thread_monitor, tid);
	}

	pthread_exit(NULL);
}


static void thread_send_message_async(t_socket_thread_callback* thread_callback) {
	pthread_t tid;
	t_socket_handler* handler = thread_callback->handler;

	if (handler->thread_monitor != NULL) {
		tid = pthread_self();
		handler->thread_monitor->subscribe(handler->thread_monitor, tid);
	}

	send_message(handler, thread_callback->header, thread_callback->buffer);
	thread_callback->callback(handler);

	thread_callback_destroy(thread_callback);

	// si una monitor de hilos, entonces le aviso que este hilo completo su ejecucion
	if (handler->thread_monitor != NULL) {
		handler->thread_monitor->unsubscribe(handler->thread_monitor, tid);
	}

	pthread_exit(NULL);
}


static void thread_handle_client(t_socket_handler* client) {
	pthread_t tid;
	int r = pthread_create(&tid, NULL, (void*)handle_client, client);

	if (r != 0) {
		client->server->error = r;

		if (client->server->on_error != NULL)
			client->server->on_error(client->server);

		return;
	}

	pthread_detach(tid);
}


static void handle_client(t_socket_handler *client) {
	pthread_t tid;
	t_socket_handler* server = client->server;
	t_thread_monitor* monitor = client->thread_monitor;

	if (monitor != NULL) {
		tid = pthread_self();
		monitor->subscribe(monitor, tid);
	}

	if (server->on_handshake == NULL || server->on_handshake(client) == CONNECTION_ACCEPTED) {
		server->on_client_connected(client);
	}
	else {
		socket_handler_disconnect(client);
		socket_handler_destroy(client);
	}

	// si usa monitor de hilos, entonces le aviso que este hilo completo su ejecucion
	if (monitor != NULL) {
		monitor->unsubscribe(monitor, tid);
	}

	pthread_exit(NULL);
}



static void handle_server(t_socket_handler *server) {
	pthread_t tid;
	char* client_ip = NULL;
	t_thread_monitor* monitor = server->thread_monitor;
	bool error = false;

	// agrego el hilo al monitor
	if (monitor != NULL) {
		tid = pthread_self();
		monitor->subscribe(monitor, tid);
	}

	server->reset(server);
	server->connected = true;

	if (server->async && server->on_listening != NULL)
		server->on_listening(server);

	while (true) {
		server->last_return = esperar_cliente(server->socket, client_ip);

		if (server->disconnecting) {
			break;
		}
		else if (server->last_return < 0) {
			server->error = abs(server->last_return);
			error = true;
			break;
		}

		t_socket_handler *client = client_handler_create(client_ip, NULL);
		client->socket = server->last_return;
		client->server = server;
		client->thread_monitor = server->thread_monitor;
		client->connected = true;
		//free(client_ip);

		if (server->async)
			thread_handle_client(client);
		else
			handle_client(client);
	}

	server->connected = false;

	// si una monitor de hilos, entonces le aviso que este hilo completo su ejecucion
	if (monitor != NULL) {
		monitor->unsubscribe(monitor, tid);
	}

	if (server->async) {
		if (error && server->on_error != NULL)
			server->on_error(server);

		else if (server->on_disconnected != NULL)
			server->on_disconnected(server);
	}

	pthread_exit(NULL);
}


static void reset_status(t_socket_handler* handler) {
	handler->last_return = -1;
	handler->error = 0;
}


static t_socket_thread_callback* thread_callback_create(t_socket_handler* handler, uint32_t header, t_buffer* buffer, void (*callback)(struct t_socket_handler*)) {
	t_socket_thread_callback* self = malloc(sizeof(t_socket_thread_callback));
	self->handler = handler;
	self->header = header;
	self->buffer = buffer;
	self->callback = callback;
	return self;
}

static void thread_callback_destroy(t_socket_thread_callback* self) {
	if (self->buffer != NULL)
		buffer_destroy(self->buffer);

	free(self);
}


static int thread_create(t_thread_monitor* monitor, pthread_t* tid, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg) {

	if (monitor == NULL) {
		int result = pthread_create(tid, attr, start_routine, arg);

		if (result != 0) {
			return result;
		}

		return pthread_detach(*tid);
	}
	else {
		return monitor->new(monitor, tid, attr, start_routine, arg);
	}
}


static void socket_handler_destroy_in_list(void* handler) {
	socket_handler_destroy((t_socket_handler*)handler);
}


/*
 * no se debe invocar a este metodo directamente.
 * Usen server_handler_create o client_handler_create
 */
static t_socket_handler* socket_handler_create(t_socket_type type, const char* ip, const char* port) {

	t_socket_handler *self = malloc(sizeof(t_socket_handler));

	self->name = NULL;
	self->type = type;
	self->socket = -1;
	self->ip = (ip != NULL? strdup(ip) : NULL);
	self->port = (port != NULL? strdup(port) : NULL);
	self->connected = false;
	self->data = queue_create();
	self->last_return = -1;
	self->error = 0;
	self->thread_monitor = NULL;
	self->server = NULL;
	self->clients = NULL;
	self->async = false;
	self->disconnecting = false;

	self->on_handshake = NULL;
	self->on_client_connected = NULL;
	self->on_disconnected = NULL;
	self->on_listening = NULL;
	self->on_error = NULL;
	self->reset = reset_status;

	return self;
}

