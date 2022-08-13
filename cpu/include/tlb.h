/*
 * tlb.h
 *
 *  Created on: 26 jun. 2022
 *      Author: utnso
 */

#ifndef TLB_H_
#define TLB_H_

#include "cpu_global.h"


typedef enum {
	AR_FIFO,
	AR_LRU
} t_alg_reemplazo;

//structs para TLB
typedef struct {
	int pagina;
	int marco;
} t_entry_tlb;

typedef struct {
	t_list*			lista_tlb;
	int 			cant_entradas;
	t_alg_reemplazo	protocolo;
	//char * 	protocolo;
} t_tlb;



//cre aun TLB, junto con su lista y protocolo de reemplazo
t_tlb * crearTLB(int cant_entradas, t_alg_reemplazo protocolo);
//destruye la TLB haciendo todos los free correspondientes
void destroyTLB(t_tlb* self);


t_entry_tlb* entry_tlb_create();
void entry_tlb_destroy(t_entry_tlb* self);


//limpiamos todos los elementos de la lista de la tlb
void limpiar_tlb(t_tlb *);
//reemplazamos un elemento de la lista acorde al protocolo de reemplazo
void reemplazar_entrada(t_tlb* self, t_entry_tlb* entrada);
//buco marco en mi tlb
int buscar_marco(t_tlb* self, int pagina);

#endif
