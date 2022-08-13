#include <stdio.h>

#include "../include/tests.h"
#include "../include/tablas.h"

//Tests

void test_tam_tabla_tablas(){
	printf("%d\n", (int)sizelist(*lista_tablas_de_tablas));
	printf("%d\n", (int)sizelist(*lista_tablas_de_paginas));

	int paginaPN = crear_tabla_primer_nivel(1024, 1);

	printf("1) %d\n", paginaPN);

	printf("tablas de tablas %d\n", (int)sizelist(*lista_tablas_de_tablas));
	printf("tablas de paginas %d\n", (int)sizelist(*lista_tablas_de_paginas));

	paginaPN = crear_tabla_primer_nivel(512, 2);

	printf("2) %d\n", paginaPN);

	printf("tablas de tablas %d\n", (int)sizelist(*lista_tablas_de_tablas));
	printf("tablas de paginas %d\n", (int)sizelist(*lista_tablas_de_paginas));

	paginaPN = crear_tabla_primer_nivel(1200, 3);

	printf("3) %d\n", paginaPN);

	printf("tablas de tablas %d\n", (int)sizelist(*lista_tablas_de_tablas));
	printf("tablas de paginas %d\n", (int)sizelist(*lista_tablas_de_paginas));
}

void test_espacio_usuario(){
	uint32_t* parametro = malloc(sizeof(uint32_t));

	escribir_marco(15, 1, 22);
	escribir_marco(17, 0, 32);
	escribir_marco(17, 1, 33);
	escribir_marco(15, 0, 23);

	leer_marco(15, 1, parametro);
	printf("valor1 = %d\n", (int)*parametro);
	leer_marco(17, 0, parametro);
	printf("valor2 = %d\n", (int)*parametro);
	leer_marco(17, 1, parametro);
	printf("valor3 = %d\n", (int)*parametro);
	leer_marco(15, 0, parametro);
	printf("valor4 = %d\n", (int)*parametro);
}

void test_swap(){
	uint32_t* numero = malloc(sizeof(uint32_t));
	*numero = 35;
	void* dato = malloc(configuracion_memoria -> tam_pagina);

	memcpy(dato, numero, sizeof(uint32_t));

	crear_archivo_swap(12, 128);
	escribir_pagina_swap(12, 2, 0, dato);

	*numero = 34;
	memcpy(dato, numero, sizeof(uint32_t));

	escribir_pagina_swap(12, 2, 1, dato);

	*numero = 33;
	memcpy(dato, numero, sizeof(uint32_t));
	escribir_pagina_swap(12, 1, 2, dato);

	*numero = 39;
	memcpy(dato, numero, sizeof(uint32_t));
	escribir_pagina_swap(12, 0, 0, dato);

	*numero = 62;
	memcpy(dato, numero, sizeof(uint32_t));
	escribir_pagina_swap(12, 1, 3, dato);

	*numero = 58;
	memcpy(dato, numero, sizeof(uint32_t));
	escribir_pagina_swap(12, 0, 3, dato);

	free(dato);



	dato = leer_pagina_swap(12, 2, 0);
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));


	dato = leer_pagina_swap(12, 2, 1);
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));


	dato = leer_pagina_swap(12, 1, 2);
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));


	dato = leer_pagina_swap(12, 1, 3);
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));

	dato = leer_pagina_swap(12, 0, 0);
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));

	dato = leer_pagina_swap(12, 0, 3);
	*numero = 10;
	memcpy(numero, dato, sizeof(uint32_t));
	printf("%d\n", (int)(*numero));
}

void test_bitmap_espacio_usuario(){
	uint32_t cantidad = configuracion_memoria->tam_memoria/configuracion_memoria->tam_pagina;

	for(uint32_t i = 0; i<cantidad; i++){
		if((registro_espacioUsuario + i)->asignado == false){
			printf("Si %d\n", i);
		}
		else{
			printf("No %d\n", i);
		}
	}
}

void test_escritura_lectura_paginas(){
	uint32_t paginaPN = iniciar_nuevo_proceso(1024, 21);
	uint32_t paginaPN2 = iniciar_nuevo_proceso(512, 23);

	escribir_en_pagina(paginaPN, 0, 0, 20);
	escribir_en_pagina(paginaPN, 0, 4, 21);
	escribir_en_pagina(paginaPN, 0, 0, 22);
	escribir_en_pagina(paginaPN, 0, 0, 23);
	escribir_en_pagina(paginaPN, 0, 0, 24);
	escribir_en_pagina(paginaPN, 0, 0, 25);
	escribir_en_pagina(paginaPN, 1, 0, 26);
	escribir_en_pagina(paginaPN, 2, 0, 27);
	escribir_en_pagina(paginaPN, 3, 0, 28);
	escribir_en_pagina(paginaPN, 4, 0, 29);
	escribir_en_pagina(paginaPN, 1, 0, 42);
	escribir_en_pagina(paginaPN, 6, 0, 30);
	escribir_en_pagina(paginaPN, 7, 0, 31);

	escribir_en_pagina(paginaPN2, 0, 0, 150);


	uint32_t resultado = leer_en_pagina(paginaPN, 0, 0);
	uint32_t resultado2 = leer_en_pagina(paginaPN, 0, 1);
	uint32_t resultado3 = leer_en_pagina(paginaPN2, 0, 0);

	printf("El archivo %d contiene: \n", (int)paginaPN);
	printf("El resultado es: %d\n", (int)resultado);
	printf("El resultado2 es: %d\n", (int)resultado2);
	printf("El archivo %d contiene: \n", (int)paginaPN2);
	printf("El resultado3 es: %d\n", (int)resultado3);

	suspender_proceso(paginaPN);


	uint32_t paginaPN3 = iniciar_nuevo_proceso(256, 25);

	escribir_en_pagina(paginaPN3, 0, 0, 75);
	escribir_en_pagina(paginaPN3, 0, 4, 76);
	escribir_en_pagina(paginaPN3, 2, 0, 77);
	escribir_en_pagina(paginaPN3, 3, 4, 78);

	uint32_t resultado4 = leer_en_pagina(paginaPN3, 3, 1);

	printf("El archivo %d contiene: \n", (int)paginaPN3);
	printf("El resultado4 es: %d\n", (int)resultado4);

	
	
	resultado = leer_en_pagina(paginaPN, 0, 4);
	resultado2 = leer_en_pagina(paginaPN, 2, 0);

	printf("El archivo %d contiene: \n", (int)paginaPN);
	printf("El resultado es: %d\n", (int)resultado);
	printf("El resultado2 es: %d\n", (int)resultado2);

	printf("\n###### fin del test ######\n");
}




void test_algoritmos_reemplazo1(){
	uint32_t paginaPN = iniciar_nuevo_proceso(1024, 11);

	escribir_en_pagina(paginaPN, 2, 0, 20);
	escribir_en_pagina(paginaPN, 3, 0, 21);
	escribir_en_pagina(paginaPN, 2, 0, 22);
	escribir_en_pagina(paginaPN, 1, 0, 23);
	escribir_en_pagina(paginaPN, 5, 0, 24);
	escribir_en_pagina(paginaPN, 2, 0, 25);
	escribir_en_pagina(paginaPN, 4, 0, 26);
	escribir_en_pagina(paginaPN, 5, 0, 27);
	escribir_en_pagina(paginaPN, 3, 0, 28);
	escribir_en_pagina(paginaPN, 2, 0, 29);
	escribir_en_pagina(paginaPN, 5, 0, 30);
	escribir_en_pagina(paginaPN, 2, 0, 21);

	printf("Algoritmo Clock con 3 marcos tiene que resultar marco[0]=3, marco[1]=2, marco[2]=5");
}


void test_algoritmos_reemplazo2(){
	uint32_t paginaPN = iniciar_nuevo_proceso(1024, 11);

	escribir_en_pagina(paginaPN, 1, 0, 0);
	escribir_en_pagina(paginaPN, 2, 0, 0);
	escribir_en_pagina(paginaPN, 3, 0, 0);
	escribir_en_pagina(paginaPN, 4, 0, 0);
	escribir_en_pagina(paginaPN, 5, 0, 0);

	suspender_proceso(paginaPN);



	leer_en_pagina(paginaPN, 2, 0);
	leer_en_pagina(paginaPN, 3, 0);
	escribir_en_pagina(paginaPN, 2, 0, 22);
	leer_en_pagina(paginaPN, 1, 0);
	leer_en_pagina(paginaPN, 5, 0);
	leer_en_pagina(paginaPN, 2, 0);
	escribir_en_pagina(paginaPN, 4, 0, 26);
	leer_en_pagina(paginaPN, 5, 0);
	leer_en_pagina(paginaPN, 3, 0);
	leer_en_pagina(paginaPN, 2, 0);
	leer_en_pagina(paginaPN, 5, 0);
	leer_en_pagina(paginaPN, 2, 0);

	printf("Algoritmo Clock-M con 3 marcos tiene que resultar marco[0]=2, marco[1]=3, marco[2]=5");
}