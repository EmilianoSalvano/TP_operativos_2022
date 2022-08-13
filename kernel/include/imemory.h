/*
 * memory.h
 *
 *  Created on: 14 jul. 2022
 *      Author: utnso
 */

#ifndef IMEMORY_H_
#define IMEMORY_H_

#include "kernel_global.h"
#include "../../shared/include/kiss.structs.h"


t_memory_module* memory_module_create();
void memory_module_destroy();

int memory_module_run();
int memory_module_stop();

int get_entry_page_table(t_pcb* pcb);
int swapp_process(t_pcb* pcb);
int free_allocated_memory(t_pcb* pcb);

#endif /* IMEMORY_H_ */
