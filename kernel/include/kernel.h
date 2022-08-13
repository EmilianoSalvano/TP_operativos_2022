#ifndef KERNEL_H
#define KERNEL_H

#include <commons/log.h>
/*
 * @NAME: kernel_run
 * @DESC: incia el servidor de consolas
 * 			Se conecta al modulo CPU
 * 			Se conecta al modulo Memoria
 * 			Inicia la planificacion de consolas
 */
int kernel_execute(t_log_level log_level, char* cfg);


/*
 * @NAME: kernel_terminate
 * @DESC:
 */
void kernel_terminate();




#endif 
