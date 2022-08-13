/*
 * socket_handlers.h
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_SOCKET_HANDLER_H_
#define INCLUDE_SOCKET_HANDLER_H_

#include <pthread.h>
#include "serialization.h"
#include "thread_monitor.h"
#include <commons/collections/queue.h>




typedef enum {
	CONNECTION_ACCEPTED = -1,
	CONNECTION_REJECTED = 1
} t_handshake_result;


/*
 * @NAME: t_socket_type
 * @DESC: tipos posibles de menajedor de sockets
*/
typedef enum {
	SOCKET_TYPE_CLIENT,
	SOCKET_TYPE_SERVER
} t_socket_type;



/*
 * @NAME: t_socket_handler
 * @DESC: manejador de sockets, sirve para conexiones de tipo servidor o cliente
 * 	Parametros:
 * 	-type: indica de que forma se esta utilizando el socket (servidor o cliente)
 * 	-socket: numero de socket asignado por el SO
 * 	-ip: si es servidor, es la ip en la que escucha conexiones. Si es cliente, es la ip del servidor al que se conecto
 * 	-port: si es servidor, es el puerto en el que escucha conexiones. Si es cliente, es el puerto del servidor al que se comecto
 * 	-connected: indica que el socket esta abierto y conectado
 * 	-data: cola de usos generales que puede ser utilizada para agregar datos o parametros al manejador
 * 	-last_return: valor del ultimo retorno de la funcion POSIX ejecutada
 *
 * 	Atributos solo de servidor
 * 	-clients: lista de clientes conectados al servidor
 * 	-handle_handshake: puntero a la funcion que ejecuta el handshake con el nuevo cliente conectado. El servidor invoca esta funcion por cada nueva conexion
 * 	-on_client_connected: el servidor invoca esta funcion por cada nueva conexion de cliente. Se ejecuta luego de que haya sido aceptado el handshake
 * 	-async: Si esta activado:
 * 			1. El servidor atiende conexiones en un hilo independiente
 * 			2. el entorno de cada cliente conectado al servidor, se ejecuta en un hilo independiente
 * 	-mutex_t_signal: mutex utilizado para proteger el acceso de multiples hilos a la variable "t_signal"
 * 	-t_signal: indica si el manejador fue señalizado para que termine la ejecucion de su bucle, si es que esta utilizandose dentro de uno
 * 				es un flag para darle a saber desde fuera del hilo, que tiene que dejar de procesar
 * 				cambiar el flag no detiene ningun proceso, solo es un aviso. es tarea del programador usarlo y detener el bucle
 *
 * 	Las variables "read only" no deberian ser modificadas por el programador. Esto puede generar un mal funcionamiento de la biblioteca
 */

typedef struct t_socket_handler {
	char*				name;
	t_socket_type		type;				// read only
	int 				socket;				// read only
	char				*ip;				// read only
	char				*port;				// read only
	bool				connected;			// read only
	t_queue				*data;
	int					error;				// read only
	int					last_return;		// read only
	t_thread_monitor	*thread_monitor;	// read only

	// solo para cliente
	struct t_socket_handler*	server;		// read only

	// solo para servidor
	bool				disconnecting;		// private
	t_list*				clients;
	void				(*reset)(struct t_socket_handler*);
	t_handshake_result	(*on_handshake)(struct t_socket_handler*);
	void				(*on_client_connected)(struct t_socket_handler*);
	void				(*on_listening)(struct t_socket_handler*);
	void				(*on_disconnected)(struct t_socket_handler*);
	void				(*on_error)(struct t_socket_handler*);
	bool				async;				// read only
	//bool				stop_server;		// read only
} t_socket_handler;



/*
 * @NAME: socket_handler_create
 * @DESC: crea e inicializa la estructura t_socket_handler
 *
 */
t_socket_handler* server_handler_create(const char* ip, const char* port);
t_socket_handler* client_handler_create(const char* ip, const char* port);


/*
 * @NAME: socket_handler_destroy
 * @DESC: destruye la estructura t_socket_handler
 * 		1. Si el socket todavia esta conectado, no hace nada y devuelve EXIT_FAILURE
 * 		2. NO destruye las conexiones clientes (para servidor). Los clientes conectados al servidor deben ser liberados por el programador
 * 			y cerrar sus conexiones adecuadamente
 *
 */
int socket_handler_destroy(t_socket_handler* self);


/*
 * @NAME: socket_handler_disconnect
 * @DESC: cierra la conexion de socket, la libera y envia la señal de proceso terminado
 * 	Errores posibles
 * 		EBADF : fd isn't a valid open file descriptor.
 *		EINTR : The close() call was interrupted by a signal; see signal(7).
 *		EIO : An I/O error occurred.
*/
int socket_handler_disconnect(t_socket_handler* self);



/*
 * @NAME: start_server
 * @DESC: inicia un socket servidor.
 * 	El servidor aun no esta escuchado conexiones!
 *
 */
t_socket_handler* start_server(const char* ip, const char* port);


/*
 * @NAME: server_listen
 * @DESC: el servidor se queda ecuchando nuevas conexiones de clientes.
 * 			Por cada nueva conexion, invoca la funcion callback on_client_connected definida en t_socker_handler
 * 			Este metodo es bloqueante!!
 *
 */
void server_listen(t_socket_handler* server);


/*
 * @NAME: server_listen_async
 * @DESC: Idem funcion "server_listen", con la diferencia que crea y se ejecuta en un hilo secundario.
 * 			La llamada a este metodo no es bloqueante
 *
 */
pthread_t server_listen_async(t_socket_handler* server);


/*
 * @NAME: connect_to_server
 * @DESC: inicia un socket cliente y estabelce la conexion con el servidor a la ip y puerto definido
 *
 */
t_socket_handler* connect_to_server(const char* ip, const char* port);


/*
 * @NAME: recieve_message
 * @DESC: gestiona la llegada de un nuevo mensaje.
 * 			El metodo es bloqueante!
 *
 */
t_package* recieve_message(t_socket_handler* handler);


/*
 * @NAME: recieve_message_async
 * @DESC: Idem "recieve_message", pero en este caso el metodo crea y se ejecuta en un hilo secundario.
 * 			Cuando termina de recibir el mensaje, invoca la funcion callback y devuelve el manejador del socket que recibe el mensaje
 * 			El mensaje se guarda en handler->data
 * 			El metodo no es bloqueante
 *
 */
pthread_t recieve_message_async(t_socket_handler* handler, void(*callback)(t_socket_handler*));


/*
 * @NAME: send_signal
 * @DESC: envia solo el codigo de señal indicado
 * 			El metodo no es bloqueante
 *
 */
int send_signal(t_socket_handler* handler, uint32_t header, uint32_t signal);


/*
 * @NAME: send_message
 * @DESC:envia solo el codigo de señal indicado
 * 			El metodo no es bloqueante
 *
 */
pthread_t send_signal_async(t_socket_handler* handler, uint32_t header, uint32_t signal, void(*callback)(t_socket_handler*));



/*
 * @NAME: send_message
 * @DESC: envia un mensaje al socket definido. Se debe indicar el codigo de mensaje a enviar y el mensaje en si (buffer)
 * 			El metodo no es bloqueante
 *
 */
int send_message(t_socket_handler *handler, uint32_t header, t_buffer* buffer);

/*
 * @NAME: send_message_async
 * @DESC: invoca a "send_message" sobre un nuevo hilo
 * 			El metodo no es bloqueante
 *
 */
pthread_t send_message_async(t_socket_handler *handler, uint32_t header, t_buffer* buffer, void(*callback)(t_socket_handler*));



#endif /* INCLUDE_SOCKET_HANDLER_H_ */
