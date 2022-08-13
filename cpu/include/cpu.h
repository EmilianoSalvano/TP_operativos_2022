/*
 * cpu.h
 *
 *  Created on: 31 jul. 2022
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <commons/log.h>

void cpu_iniciar(t_log_level log_level, char* cfg);
void cpu_detener();

#endif /* INCLUDE_CPU_H_ */
