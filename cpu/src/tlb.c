/*
 * tlb.c
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#include <stdio.h>
#include <commons/string.h>


#include "../include/tlb.h"
#include "../include/cpu_global.h"

static void struct_tlb_destroy_in_list(void* entry);

t_log* logger_cpu;


t_entry_tlb* entry_tlb_create() {
	t_entry_tlb* self = malloc(sizeof(t_entry_tlb));
	self->marco = -1;
	self->pagina = -1;

	return self;
}

void entry_tlb_destroy(t_entry_tlb* self) {
	free(self);
}

t_tlb * crearTLB(int cant_entradas, t_alg_reemplazo protocolo)
{
	t_tlb* self = malloc(sizeof(t_tlb));
	self->cant_entradas = cant_entradas; //configuracion->config_tlb.cant_entradas;
	self->lista_tlb = list_create();
	self->protocolo = protocolo;  //string_duplicate(configuracion->config_tlb.reemplazo);

	return self;
}

void destroyTLB(t_tlb* self)
{
	list_destroy_and_destroy_elements(self->lista_tlb, struct_tlb_destroy_in_list);
	free(self);
}

void limpiar_tlb(t_tlb* self)
{
	list_clean_and_destroy_elements(self->lista_tlb, struct_tlb_destroy_in_list);
}


void reemplazar_entrada(t_tlb* self, t_entry_tlb* entrada)
{
	t_entry_tlb * aux;

	if (list_size(self->lista_tlb) < self->cant_entradas)
	{
		list_add(self->lista_tlb, entrada);
	}
	//else if (string_equals_ignore_case("FIFO", TLB->protocolo))
	else if (self->protocolo == AR_FIFO)
	{
		aux = list_remove(self->lista_tlb, 0);
		log_info(logger_cpu, "Remplazando marco [%d] y pagina [%d], por marco [%d] y pagina [%d]", aux->marco, aux->pagina, entrada->marco, entrada->pagina);
		entry_tlb_destroy((t_entry_tlb*)aux);

		list_add(self->lista_tlb, entrada);
	}
	//else if (string_equals_ignore_case("LRU", TLB->protocolo))
	else if (self->protocolo == AR_LRU)
	{
		aux = list_remove(self->lista_tlb, 0);
		log_info(logger_cpu, "Remplazando marco [%d] y pagina [%d], por marco [%d] y pagina [%d]", aux->marco, aux->pagina, entrada->marco, entrada->pagina);
		entry_tlb_destroy((t_entry_tlb*)aux);

		list_add(self->lista_tlb, entrada);
	}
}

int buscar_marco(t_tlb* self, int pagina)
{
	int marco = -1, i, ubicacion;
	t_entry_tlb * buscarPagina;

	for (i = 0; i < list_size(self->lista_tlb); i++)
	{
		buscarPagina = list_get(self->lista_tlb, i);

		if (buscarPagina->pagina == pagina)
		{
			marco = buscarPagina->marco;
			ubicacion = i;
			break;
		}
	}

	// Facilita el reemplazo de LRU
	if (marco != -1 && self->protocolo == AR_LRU)
	{
		//t_struct_tlb * aux = list_get(TLB->lista_tlb, ubicacion);
		t_entry_tlb * aux = list_remove(self->lista_tlb, ubicacion);
		list_add(self->lista_tlb, aux);
	}

	return marco;
}



static void struct_tlb_destroy_in_list(void* entry) {
	entry_tlb_destroy((t_entry_tlb*)entry);
}
