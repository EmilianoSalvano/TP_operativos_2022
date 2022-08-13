/*
 * hilos.c
 *
 *  Created on: 22 jul. 2022
 *      Author: utnso
 */
#include "../include/hilos.h"

void iniciar_hilo_pcb()
{
	int i = 0;
	while(i<10)
	{
		t_pcb * pcb_recibido = produce_pcb();
		printf("EJECUTANDO PCB %d\n \n", i);
		ejecutar_pcb(pcb_recibido);
		printf("FINALIZO PCB %d\n \n", i);
		//liberamos al meomoria
		pcb_destroy(pcb_recibido);
		i++;
	}
}

void iniciar_hilo_interrupt()
{
	//no_haya_interrupcion = convertir_package(paquete_recibido);
}
