/*
 * main.c
 *
 *  Created on: 5 jun. 2022
 *      Author: utnso
 */

/*
 * Puerto en uso. cualquiera de estos comandos

	sudo lsof -i -P -n | grep LISTEN
	sudo netstat -tulpn | grep LISTEN
	sudo ss -tulpn | grep LISTEN
	sudo lsof -i:22 ## see a specific port such as 22 ##
	sudo nmap -sTU -O IP-address-Here
*/


/*
 * procesos e hilos

 https://unixhealthcheck.com/blog?id=465#:~:text=Using%20the%20top%20command,by%20pressing%20'H'%20key.

 	 ps -T -p <pid>		remplazar <pid> por el id del proceso
 */

#include "../include/main.h"




/*
 * Habilitar los tests que se quieran correr
 */
int main(void) {

	//test_instruccion_to_string();
	//test_pcb_to_string();

	//test_selializar_paquete();
	//test_serializar_instrucciones();
	test_serializar_pcb();
	//test_serializar_programa();
	//test_serializar_direccion();
	//test_serializar_operacion();
	//test_files();

	//test_serializar_code();

	//test_monitor();
	//test_subscribe_monitor();
	//test_hilos();
	//test_socket();
	//test_socket_handler();

	//test_simple_socket();

	//timer_test();
	//timer_cancel_test();

	//pthread_exit(NULL);

	return EXIT_SUCCESS;
}
