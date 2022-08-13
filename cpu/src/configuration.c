/*
 * config.c
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>

#include "../include/configuration.h"
#include <commons/config.h>
#include <commons/string.h>

t_config_cpu * create_configuration(char* path)
{
	t_config * config = config_create(path);
	t_config_cpu * self = malloc(sizeof(t_config_cpu));

	//configuraciones para la TLB
	self->cant_entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
	self->algoritmo_reemplazo_tlb = string_duplicate(config_get_string_value(config, "REEMPLAZO_TLB"));

	//configuraciones para el CPU
	self->retardo_NO_OP = config_get_int_value(config, "RETARDO_NOOP");
	self->ip_memoria = string_duplicate(config_get_string_value(config, "IP_MEMORIA"));
	self->puerto_memoria = string_duplicate(config_get_string_value(config, "PUERTO_MEMORIA"));
	self->puerto_escucha_dispath = string_duplicate(config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH"));
	self->puerto_escucha_interrupt = string_duplicate(config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT"));

	config_destroy(config);
	return self;
}


void destroy_configuration(t_config_cpu* self)
{
	// TODO: memory leak
	free(self->algoritmo_reemplazo_tlb);
	free(self->ip_memoria);
	free(self->puerto_escucha_dispath);
	free(self->puerto_escucha_interrupt);
	free(self->puerto_memoria);

	free(self);
}

