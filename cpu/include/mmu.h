/*
 * mmu.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#ifndef MMU_H_
#define MMU_H_

//#include "cpu_global.h"
//#include "tlb.h"

#include <stdbool.h>
#include "../include/tlb.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.structs.h"

//structs para MMU
typedef struct
{
	t_tlb * TLB;
	t_setup_memoria* config_memoria;
} t_mmu;


typedef struct
{
	int num_pag;
	int marco;
	int desplazamiento;
	bool acierto;
} t_dir_fisica;


typedef struct
{
	int primer_nivel;
	int segundo_nivel;
	int num_pag;
} t_datos_para_marco;



t_mmu* create_mmu(t_setup_memoria* config_memoria, int entradas_tlb, t_alg_reemplazo protocolo_tlb);
void destroy_mmu(t_mmu* self);

t_dir_fisica direccionFisica(t_mmu* self, t_pcb* pcb, int dir_logica);

#endif
