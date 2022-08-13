/*
 * cicloInstruccion.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#ifndef CICLOINSTRUCCION_H_
#define CICLOINSTRUCCION_H_

#include <stdbool.h>
#include "../../shared/include/kiss.structs.h"
#include "../include/mmu.h"

t_mmu* MMU;

bool ejecutar_pcb(t_pcb* pcb);

#endif
