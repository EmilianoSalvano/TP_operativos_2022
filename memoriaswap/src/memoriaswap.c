/*
 * memoriaswap.c
 *
 *  Created on: 31 jul. 2022
 *      Author: utnso
 */

#include <semaphore.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

#include "../include/memoriaswap.h"
#include "../../shared/include/kiss.error.h"


bool CPU_CONECTADO = false;
bool KERNEL_CONECTADO = false;

t_log* logger_memoria;
sem_t sem_request;



/*
 Encontre estos warnings

1. parece que este puntero no se usa
 ../src/tablas.c:342:5: warning: value computed is not used [-Wunused-value]
     *puntero ++;

2. la funcion es de tipo void* pero no devuelve nada. sacale el *
../src/tablas.c: In function ‘leer_marco_completo’:
../src/tablas.c:139:1: warning: control reaches end of non-void function [-Wreturn-type]

3. no se porque no me esta teniendo en cuenta el #include <math.h> en tablas.h, no se si te pasa a vos. por las dudas te aviso
/home/utnso/git/tp-2022-1c-Los-o-os/memoriaswap/Debug/../src/tablas.c:60: undefined reference to `ceil'
 */



static void handle_client(t_socket_handler* client);
static t_handshake_result server_handshake(t_socket_handler* server);
static t_handshake_result cpu_handshake(t_socket_handler* client);

static void handle_client(t_socket_handler* client);
static void handle_cpu(t_socket_handler* clien);
static void handle_kernel(t_socket_handler* client);
static void message_error(t_socket_handler* handler, t_signal signal);

static void server_listening(t_socket_handler* server);
static void server_disconnected(t_socket_handler* server);
static void server_error(t_socket_handler* server);


int memoria_iniciar(t_log_level log_level, char* cfg) {

	logger_memoria = log_create("memoria.log", "memoria", true, log_level);
	configurar_memoria(cfg);

	sem_init(&sem_request, 0, 1);

	lista_tablas_de_paginas = malloc(sizeof(List));
	lista_tablas_de_tablas = malloc(sizeof(List));
	initlist(lista_tablas_de_tablas);
	initlist(lista_tablas_de_paginas);

	//test_tam_tabla_tablas();
	//test_espacio_usuario();
	//test_swap();
	//test_bitmap_espacio_usuario();
	//test_escritura_lectura_paginas();
	//test_algoritmos_reemplazo1();
	//test_algoritmos_reemplazo2();


	t_socket_handler* server_cpu_kernel = start_server("127.0.0.1", configuracion_memoria -> puerto_escucha);

	server_cpu_kernel->on_client_connected = handle_client;

	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	server_cpu_kernel->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	server_cpu_kernel->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	server_cpu_kernel->on_error = server_error;
	// handshake
	server_cpu_kernel->on_handshake = server_handshake;

	server_listen_async(server_cpu_kernel);


	//log_destroy(logger_memoria);

	pthread_exit(NULL);
}


static t_handshake_result server_handshake(t_socket_handler* client) {
	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(client);

	if (client->error != 0) {
		log_error(logger_memoria, "Ocurrio un error al recibir el mensaje del kernel. error:[%d] msg:[%s]. Conexion rechazada de socket:[%d]",
				client->error, errortostr(client->error), client->socket);
	}
	else if (package->header != MC_PROCESS_CODE) {
		log_error(logger_memoria, "Codigo:[%d] de paquete incorrecto para handshake. Conexion rechazada de socket:[%d]", package->header, client->socket);
	}
	else {
		t_process_code process_code = deserialize_process_code(package->payload);

		if (process_code == PROCESS_KERNEL) {
			log_info(logger_memoria, "Conexion aceptada con kernel en socket:[%d]", client->socket);

			KERNEL_CONECTADO = true;

			t_process_code* code = malloc(sizeof(t_process_code));
			*code = process_code;
			queue_push(client->data, code);

			result = CONNECTION_ACCEPTED;
			send_signal(client, MC_SIGNAL, SG_CONNECTION_ACCEPTED);
		}
		else if (process_code == PROCESS_CPU) {
			result = cpu_handshake(client);

			if (result == CONNECTION_ACCEPTED) {
				CPU_CONECTADO = true;
				t_process_code* code = malloc(sizeof(t_process_code));
				*code = process_code;
				queue_push(client->data, code);
			}
		}
		else {
			log_error(logger_memoria, "Codigo:[%d] de proceso para handshake incorrecto. Conexion rechazada de socket:[%d]", process_code, client->socket);
		}
	}

	buffer_destroy(package->payload);
	package_destroy(package);

	/*
	if (result == CONNECTION_REJECTED && client->error != SKT_CONNECTION_LOST)
		send_signal(client, MC_SIGNAL, SG_CONNECTION_REJECTED);
	 */
	return result;
}



static t_handshake_result cpu_handshake(t_socket_handler* client) {

	t_setup_memoria* setup = setup_memoria_create();
	setup->entradas_por_tabla = configuracion_memoria->entradas_por_tabla;
	setup->tam_pagina = configuracion_memoria->tam_pagina;

	t_buffer* buffer = serializar_setup_memoria(setup);
	setup_memoria_destroy(setup);

	send_message(client, MC_MEMORY_SETUP, buffer);
	buffer_destroy(buffer);

	if (client->error != 0) {
		log_error(logger_memoria, "error enviando setup de memoria a cpu. error:[%d] msg:[%s]", client->error, errortostr(client->error));
		return CONNECTION_REJECTED;
	}

	return CONNECTION_ACCEPTED;
}




static void handle_client(t_socket_handler* client){
	t_process_code* process_code = queue_pop(client->data);

	if (*process_code == PROCESS_CPU) {
		handle_cpu(client);
	}
	else if (*process_code == PROCESS_KERNEL) {
		handle_kernel(client);
	}
	else {
		log_error(logger_memoria, "codigo:[%d] de proceso incorrecto. conexion finalizada", *process_code);
		socket_handler_disconnect(client);
		socket_handler_destroy(client);
	}

	/*
	if (!CPU_CONECTADO && !KERNEL_CONECTADO) {
		socket_handler_disconnect(client->server);
	}
	*/

	free(process_code);
}


// hilo secundario
static void handle_cpu(t_socket_handler* client) {
	bool connected = true;

	while(connected) {
		t_package* package = recieve_message(client);

		sem_wait(&sem_request);

		if (client->error != 0) {
			if (client->error == SKT_CONNECTION_LOST) {
				log_error(logger_memoria, "CPU desconectado. error:[%d] msg:[%s]", client->error, errortostr(client->error));
				connected = false;
			}
			else {
				log_error(logger_memoria, "error al recibir el mensaje de CPU. error:[%d] msg:[%s]", client->error, errortostr(client->error));
				message_error(client, SG_ERROR_PACKAGE);
			}
		}
		else {
			uint32_t numero_pagina;
			uint32_t numero_tabla_tablas;
			uint32_t numero_marco;
			//uint32_t resultado;
			t_operacion_EU* operacion;
			//t_operacion_EU* operacion_lectura;
			t_direccion* direccion;
			uint32_t resultado;

			log_debug(logger_memoria, "solicitud de cpu. operacion:[%d]", package->header);

			switch (package->header) {
				case MC_ESCRIBIR_MARCO:
					//t_operacion_EU* operacion_escritura = malloc(sizeof(t_operacion_EU));
					//deserializar_operacion_EU(package->payload, operacion_escritura);

					operacion = deserializar_operacion_EU(package->payload);

					log_debug(logger_memoria, "MC_ESCRIBIR_MARCO :: pid:[%d] numero_marco:[%d] desplazamiento:[%d] dato:[%d]",
								operacion->pid, operacion->numero_marco, operacion->desplazamiento, operacion->dato);

					numero_pagina = (registro_espacioUsuario + operacion->numero_marco)->pagina;
					numero_tabla_tablas = (registro_espacioUsuario + operacion->numero_marco)->tabla_tablas;

					resultado = escribir_en_pagina(numero_tabla_tablas, numero_pagina, operacion->desplazamiento, operacion->dato);

					// TODO: resultado siempre es 1. hay que modificar la funcion "escribir_en_pagina"
					if (resultado == EXIT_SUCCESS)
						send_signal(client, MC_SIGNAL, SG_OPERATION_SUCCESS);
					else
						send_signal(client, MC_SIGNAL, SG_OPERATION_FAILURE);

					//free(operacion_escritura);
					operacion_eu_destroy(operacion);
					break;

				case MC_LEER_MARCO:
					//t_operacion_EU* operacion_lectura = malloc(sizeof(t_operacion_EU));
					//deserializar_operacion_EU(package->payload, operacion_lectura);

					operacion = deserializar_operacion_EU(package->payload);

					log_debug(logger_memoria, "solicitud MC_LEER_MARCO :: pid:[%d] numero_marco:[%d] desplazamiento:[%d] dato:[%d]",
									operacion->pid, operacion->numero_marco, operacion->desplazamiento, operacion->dato);

					numero_pagina = (registro_espacioUsuario + operacion->numero_marco)->pagina;
					numero_tabla_tablas = (registro_espacioUsuario + operacion->numero_marco)->tabla_tablas;

					log_debug(logger_memoria, "calculo MC_LEER_MARCO :: numero_pagina:[%d] numero_tabla_tablas:[%d]", numero_pagina, numero_tabla_tablas);

					resultado = leer_en_pagina(numero_tabla_tablas, numero_pagina, operacion->desplazamiento);
					send_signal(client, MC_LEER_MARCO, resultado);

					log_debug(logger_memoria, "MC_LEER_MARCO :: pid:[%d] numero_marco:[%d] desplazamiento:[%d] dato:[%d] resultado:[%d]",
										operacion->pid, operacion->numero_marco, operacion->desplazamiento, operacion->dato, resultado);

					operacion_eu_destroy(operacion);
					//free(operacion_lectura);
					break;

				case MC_BUSCAR_TABLA_DE_PAGINAS:
					//t_direccion* direccion_primer_nivel = malloc(sizeof(t_direccion));
					//deserializar_direccion(package->payload, direccion_primer_nivel);

					// recibe el id de tabla de primer nivel y la pagina

					direccion = deserializar_direccion(package->payload);

					log_debug(logger_memoria, "solicitud MC_BUSCAR_TABLA_DE_PAGINAS :: pid:[%d] primer_nivel:[%d] segundo_nivel:[%d] pagina:[%d]",
							direccion->pid, direccion->tabla_primer_nivel, direccion->tabla_segundo_nivel, direccion->pagina);

					// devuelve el id de la tabla de segundo nivel
					int segundo_nivel = numero_de_tabla(direccion->pagina);
					send_signal(client, MC_BUSCAR_TABLA_DE_PAGINAS, segundo_nivel);

					log_debug(logger_memoria, "retorno MC_BUSCAR_TABLA_DE_PAGINAS :: numero de tabla:[%d]", segundo_nivel);

					/*
					log_debug(logger_memoria, "MC_BUSCAR_TABLA_DE_PAGINAS :: pid:[%d] primer_nivel:[%d] segundo_nivel:[%d] pagina:[%d]",
												direccion->pid, direccion->tabla_primer_nivel, segundo_nivel, direccion->pagina);
					*/

					direccion_destroy(direccion);
					//free(direccion_primer_nivel);
					break;

				case MC_BUSCAR_MARCO:
					//t_direccion* direccion_segundo_nivel = malloc(sizeof(t_direccion));
					//deserializar_direccion(package->payload, direccion_segundo_nivel);

					direccion = deserializar_direccion(package->payload);

					log_debug(logger_memoria, "solicitud MC_BUSCAR_MARCO :: pid:[%d] primer_nivel:[%d] segundo_nivel:[%d] pagina:[%d]",
									direccion->pid, direccion->tabla_primer_nivel, direccion->tabla_segundo_nivel, direccion->pagina);

					numero_marco = buscar_marco_de_pagina(direccion->tabla_primer_nivel, direccion->pagina);
					send_signal(client, MC_BUSCAR_MARCO, numero_marco);

					log_debug(logger_memoria, "MC_BUSCAR_MARCO :: pid:[%d] primer_nivel:[%d] segundo_nivel:[%d] pagina:[%d] marco:[%d]",
											direccion->pid, direccion->tabla_primer_nivel, direccion->tabla_segundo_nivel, direccion->pagina, numero_marco);

					direccion_destroy(direccion);
					//free(direccion_segundo_nivel);
					break;

				default:
					log_error(logger_memoria, "codigo:[%d] de mensaje incorrecto recibidod de CPU", package->header);
					message_error(client, SG_CODE_NOT_EXPECTED);
					break;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);

		sem_post(&sem_request);
	}

	socket_handler_destroy(client);
}



static void handle_kernel(t_socket_handler* client) {
	bool connected = true;
	uint32_t numero_tabla_tablas;
	t_page_entry_request* request;

	while(connected) {
		t_package* package = recieve_message(client);

		sem_wait(&sem_request);

		if (client->error != 0) {
			if (client->error == SKT_CONNECTION_LOST) {
				log_error(logger_memoria, "CPU desconectado. error:[%d] msg:[%s]", client->error, errortostr(client->error));
				connected = false;
			}
			else {
				log_error(logger_memoria, "error al recibir el mensaje de CPU. error:[%d] msg:[%s]", client->error, errortostr(client->error));
				message_error(client, SG_ERROR_PACKAGE);
			}
		}
		else {
			switch (package->header) {
				case MC_NUEVO_PROCESO:
					request = deserializar_page_entry_request(package->payload);

					numero_tabla_tablas = iniciar_nuevo_proceso(request->process_size, request->pcb_id);
					page_entry_request_destroy(request);

					log_debug(logger_memoria, "MC_NUEVO_PROCESO :: pid:[%d] process_size:[%d] tabla de tabla:[%d]",
							request->pcb_id, request->process_size, numero_tabla_tablas);

					send_signal(client, MC_NUEVO_PROCESO, numero_tabla_tablas);
					break;

				case MC_SUSPENDER_PROCESO:
					numero_tabla_tablas = deserialize_uint32t(package->payload);

					log_debug(logger_memoria, "MC_SUSPENDER_PROCESO :: numero_tabla_tablas:[%d]", numero_tabla_tablas);

					if(suspender_proceso(numero_tabla_tablas))
						send_signal(client, MC_SUSPENDER_PROCESO, SG_OPERATION_SUCCESS);
					else
						send_signal(client, MC_SUSPENDER_PROCESO, SG_OPERATION_FAILURE);
					break;

				case MC_FINALIZAR_PROCESO:
					numero_tabla_tablas = deserialize_uint32t(package->payload);

					log_debug(logger_memoria, "MC_SUSPENDER_PROCESO :: numero_tabla_tablas:[%d]", numero_tabla_tablas);

					if(finalizar_proceso(numero_tabla_tablas))
						send_signal(client, MC_FINALIZAR_PROCESO, SG_OPERATION_SUCCESS);
					else
						send_signal(client, MC_FINALIZAR_PROCESO, SG_OPERATION_FAILURE);
					break;

				default:
					log_error(logger_memoria, "codigo:[%d] de mensaje incorrecto recibido de kernel", package->header);
					message_error(client, SG_CODE_NOT_EXPECTED);
					break;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);

		sem_post(&sem_request);
	}

	socket_handler_disconnect(client);
}



static void message_error(t_socket_handler* handler, t_signal signal) {
	send_signal(handler, MC_SIGNAL, signal);
}


static void server_listening(t_socket_handler* server) {
	log_info(logger_memoria, "server_ready :: servidor preparado y escuchando ...");
}

static void server_disconnected(t_socket_handler* server) {
	log_info(logger_memoria, "server_close :: el servidor ha terminado su ejecucion");
	socket_handler_destroy(server);
}

static void server_error(t_socket_handler* server) {
	log_info(logger_memoria, "server_error :: ocurrio un error durante la ejecucion del servicio de conexiones :: error:[%d] msg:[%s]",
			server->error, errortostr(server->error));

	if (!server->connected)
		socket_handler_destroy(server);
}
