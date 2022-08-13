/*
 * cpu.h
 *
 *  Created on: 10 jul. 2022
 *      Author: utnso
 */

#ifndef ICPU_H_
#define ICPU_H_

#include "kernel_global.h"

t_cpu_module* cpu_module_create();
void cpu_module_destroy();

int cpu_module_run();
int cpu_module_stop();

int cpu_dispatch_pcb(t_pcb* pcb);
void cpu_listen_dispatch_async();
int cpu_interrupt();

#endif /* ICPU_H_ */
