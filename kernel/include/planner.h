/*
 * planificador.h
 *
 *  Created on: 28 may. 2022
 *      Author: utnso
 */

#ifndef PLANNER_H_
#define PLANNER_H_

#include "kernel_global.h"
#include "../../shared/include/socket_handler.h"

t_planner* planner_create();
void planner_destroy();

int planner_run();
int planner_stop();

void ltp_new_process(t_socket_handler* handler, t_program* program);


#endif /* PLANNER_H_ */




