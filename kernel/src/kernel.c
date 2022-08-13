#include <stdlib.h>
//#include <stdio.h>
#include <string.h>
//#include <pthread.h>
//#include <signal.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/string.h>


#include "../../shared/include/socket_handler.h"

#include "../include/kernel_global.h"
#include "../include/kernel.h"
#include "../include/iconsole.h"
#include "../include/icpu.h"
#include "../include/imemory.h"
#include "../include/planner.h"


t_log* logger;
t_kernel* kernel;


static int kernel_create(char* config_path);
static int kernel_destroy();
static void kernel_config_destroy(t_kernel_config* self);
static t_kernel_config* kernel_config_create(char* path);
static void kernel_error();


/**************************** Public *************************************/

void kernel_terminate() {
	// TODO: terminar el metodo

	/*
	 * 1. enviar la señal al planificador y esperar que termine. El planificador
	 * 		tiene que ocuparse de enviar todos los procesos a EXIT para que puedan avisar la consolas correspondientes
	 * 2. terminar las conexiones con CPU y MEMORIA
	 * 3. terminar la conexion del servidor de CONSOLAS y aguardar la desconexion
	 * 4. destruir todo y salir
	 */

	// por ahira voy a usar semaforos
	kernel->halt = true;

	console_module_stop();

	// TODO: desconectar el planificador
	planner_stop();

	cpu_module_stop();

	memory_module_stop();

	//usleep(30000000);
	//kernel_destroy(kernel);

	log_info(logger, "Programa terminado :)");
	log_destroy(logger);
}

//int kernel_execute(t_log_level log_level, char* config_path)
int kernel_execute(t_log_level log_level, char* cfg) {

	logger = log_create(ARCHIVO_LOG, NOMBRE_PROCESO, true, log_level);
	log_info(logger, "Inicio de ejecucion de kernel ...");

	if (kernel_create(cfg) != 0) {
		log_error(logger, "Error al iniciar el kernel. No se pudieron iniciar las estructuras de control");
		return EXIT_FAILURE;
	}

	// Monitor de hilos
	// TODO: ver qe hacer con el monitor. Incluir en el kernel si se utiliza
	//thread_monitor = thread_monitor_initialize();
	// asigno el monitor de hilos para que vaya monitoreando todos los hilos que se crean dentro del entorno de la biblioteca de sockets
	//kernel->console_server->thread_monitor = thread_monitor;


	// Memoria
		if (memory_module_run() != 0)
			return EXIT_FAILURE;

	// CPU
	if (cpu_module_run() != 0)
		return EXIT_FAILURE;


	// planificador
	 if (planner_run() != 0) {
		 return EXIT_FAILURE;
	 }

	// servidor de consolas
	if (console_module_run() != 0)
		return EXIT_FAILURE;

	//log_info(logger, "Kernel iniciado :)");
	return EXIT_SUCCESS;
}



/**************************** Private *************************************/

static int kernel_create(char* config_path) {
	kernel = malloc(sizeof(t_kernel));
	kernel->config = NULL;
	kernel->console = NULL;
	kernel->memory = NULL;
	kernel->cpu = NULL;
	kernel->planner = NULL;
	kernel->halt = false;
	kernel->monitor = thread_monitor_create();

	kernel->config = kernel_config_create(config_path);

	if (kernel->config == NULL || !kernel->config->loaded)
		return EXIT_FAILURE;

	kernel->console = console_module_create();
	kernel->cpu = cpu_module_create();
	kernel->memory = memory_module_create();
	kernel->planner = planner_create();
	kernel->error = kernel_error;

	return EXIT_SUCCESS;
}


static int kernel_destroy() {
	if (kernel == NULL)
		return EXIT_SUCCESS;

	if (kernel->monitor != NULL)
		thread_monitor_destroy(kernel->monitor);

	if (kernel->config != NULL)
		kernel_config_destroy(kernel->config);

	if (kernel->console != NULL)
		console_module_destroy();

	if (kernel->memory != NULL)
		memory_module_destroy();

	if (kernel->cpu != NULL)
		cpu_module_destroy();

	if (kernel->planner != NULL)
		planner_destroy();

	free(kernel);

	return EXIT_SUCCESS;
}


static void kernel_config_destroy(t_kernel_config* self) {
	if (self->ip_cpu != NULL)
		free(self->ip_cpu);

	if (self->ip_memoria != NULL)
		free(self->ip_memoria);

	if (self->puerto_cpu_interrupt != NULL)
		free(self->puerto_cpu_interrupt);

	if (self->puerto_cpu_dispatch != NULL)
		free(self->puerto_cpu_dispatch);

	if(self->puerto_escucha != NULL)
		free(self->puerto_escucha);

	if (self->puerto_memoria != NULL)
		free(self->puerto_memoria);

	free(self);
}


static t_kernel_config* kernel_config_create(char* path) {

	bool checked = true;
	const char* KEY_NOT_FOUND = "La clave '%s' no existe en el archivo de configuracion";

	t_kernel_config* self = malloc(sizeof(t_kernel_config));
	self->loaded = false;
	self->ip_memoria = NULL;
	self->puerto_memoria = NULL;
	self->ip_cpu = NULL;
	self->puerto_cpu_dispatch = NULL;
	self->puerto_cpu_interrupt = NULL;
	self->puerto_escucha = NULL;


	t_config* file_config = config_create(path);

	if (file_config == NULL) {
		log_error(logger, "No se pudo abrir el archivo de configuracion. Revise el path");
		return NULL;
	}

	if (!config_has_property(file_config, KEY_IP_MEMORIA)) {
		log_error(logger, KEY_NOT_FOUND, KEY_IP_MEMORIA);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_PUERTO_MEMORIA)) {
		log_error(logger, KEY_NOT_FOUND, KEY_PUERTO_MEMORIA);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_IP_CPU)) {
		log_error(logger, KEY_NOT_FOUND, KEY_IP_CPU);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_PUERTO_CPU_DISPATCH)) {
		log_error(logger, KEY_NOT_FOUND, KEY_PUERTO_CPU_DISPATCH);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_PUERTO_CPU_INTERRUPT)) {
		log_error(logger, KEY_NOT_FOUND, KEY_PUERTO_CPU_INTERRUPT);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_PUERTO_ESCUCHA)) {
		log_error(logger, KEY_NOT_FOUND, KEY_PUERTO_ESCUCHA);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_ALGORITMO_PLANIFICACION)) {
		log_error(logger, KEY_NOT_FOUND, KEY_ALGORITMO_PLANIFICACION);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_ESTIMACION_INICIAL)) {
		log_error(logger, KEY_NOT_FOUND, KEY_ESTIMACION_INICIAL);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_ALFA)) {
		log_error(logger, KEY_NOT_FOUND, KEY_ALFA);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_GRADO_MULTIPROGRAMACION)) {
		log_error(logger, KEY_NOT_FOUND, KEY_GRADO_MULTIPROGRAMACION);
		checked = false;
	}

	if (!config_has_property(file_config, KEY_TIEMPO_MAXIMO_BLOQUEADO)) {
		log_error(logger, KEY_NOT_FOUND, KEY_TIEMPO_MAXIMO_BLOQUEADO);
		checked = false;
	}

	self->ip_memoria = strdup(config_get_string_value(file_config, KEY_IP_MEMORIA));
	self->puerto_memoria = strdup(config_get_string_value(file_config, KEY_PUERTO_MEMORIA));
	self->ip_cpu = strdup(config_get_string_value(file_config, KEY_IP_CPU));
	self->puerto_cpu_dispatch = strdup(config_get_string_value(file_config, KEY_PUERTO_CPU_DISPATCH));
	self->puerto_cpu_interrupt = strdup(config_get_string_value(file_config, KEY_PUERTO_CPU_INTERRUPT));
	self->puerto_escucha = strdup(config_get_string_value(file_config, KEY_PUERTO_ESCUCHA));

	char* algoritmo = config_get_string_value(file_config, KEY_ALGORITMO_PLANIFICACION);

	if (string_equals_ignore_case(algoritmo, CFG_VAL_ALGORITMO_FIFO))
		self->algoritmo_planificacion = pln_FIFO;
	else if (string_equals_ignore_case(algoritmo, CFG_VAL_ALGORITMO_SRT))
		self->algoritmo_planificacion = pln_SRT;
	else {
		log_error(logger, "La configuracion para '%s=%s' no es valda. No es un algoritmo conocido", KEY_ALGORITMO_PLANIFICACION, algoritmo);
		checked = false;
	}

	self->estimacion_inicial = config_get_int_value(file_config, KEY_ESTIMACION_INICIAL);
	self->alfa = config_get_double_value(file_config, KEY_ALFA);


	self->grado_multiprogramacion = config_get_int_value(file_config, KEY_GRADO_MULTIPROGRAMACION);
	self->tiempo_maximo_bloqueado = config_get_int_value(file_config, KEY_TIEMPO_MAXIMO_BLOQUEADO);
	self->loaded = checked;

	config_destroy(file_config);

	return self;
}


static void kernel_error() {
	kernel->halt = true;

	// TODO: llamar al terminate!
}
