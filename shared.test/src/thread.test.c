/*
 * thread.test.c
 *
 *  Created on: 4 jun. 2022
 *      Author: utnso
 */

#include "../include/thread.test.h"

static void incrementar_valor();
static void on_thread_terminate();


// http://profesores.elo.utfsm.cl/~agv/elo330/2s07/lectures/POSIX_Threads.html


pthread_mutex_t mutex_contador;
pthread_mutex_t mutex_finalizador;
t_test* test;
int contador;

static void incrementar_valor() {
	for (int i = 0; i < 500; i++) {
		pthread_mutex_lock(&mutex_contador);
		contador++;
		test_fprint(test, "contador: %d\n", contador);
		pthread_mutex_unlock(&mutex_contador);
	}

	on_thread_terminate();
	pthread_exit(NULL);
}


static void on_thread_terminate() {
	static int cantidad_hilos = 0;

	pthread_mutex_lock(&mutex_contador);
	test_fprint(test, "Hilo:%lu Terminado\n", pthread_self());
	cantidad_hilos++;

	// es 2, porque cree solo dos hilos!
	if (cantidad_hilos >= 2) {
		test_close(test);
		pthread_mutex_destroy(&mutex_contador);
	}

	pthread_mutex_unlock(&mutex_contador);
}

/*
 * @NAME: test_hilos
 * @DESC: Crea dos hilos que utilizan una variable compartida. Cada uno de estos hilos incrementa la variable en 2000.
 * La variable esta protegida por un semforo mutex para que el valor final sea de 4000, 2000 por cada hilo.
 */
void test_hilos() {
	test = test_open_async("test_hilos");
	contador = 0;
	pthread_mutex_init(&mutex_contador, NULL);

	pthread_t hilo_1;
	pthread_t hilo_2;

	pthread_create(&hilo_1, NULL, (void*)incrementar_valor, NULL);
	pthread_create(&hilo_2, NULL, (void*)incrementar_valor, NULL);

	// probar con join y luego con detach

	// con join, el hilo principal se queda esperando que terminen los hijos, pero tambien se bloquea
	// todo lo que este abajo del join se va a ejecutar cuando los hilos terminen
	// pthread_join(hilo_1, NULL);
	// pthread_join(hilo_2, NULL);

	// con detach, el hilo funciona como si fuera un proceso independiente.
	// El hilo principal no espera y puede continuar su ejecucion, pero tambien podria terminar y cerrarse y eso mata a todos los hilos hijos
	pthread_detach(hilo_1);
	pthread_detach(hilo_2);
	// le doy un poquito de tiempo (1/100 seg) para que al menos algun hilo llegue a hacer algo
	// si usan el pthread_exit(NULL) no hace falta
	//usleep(10000);

	test_print(test, "Fin de hilo principal\n");
	//pthread_mutex_destroy(&mutex);

	//test_close(test);

	// de esta forma le decimos al proceso que al cerrarse lo haga de distinta forma,
	// no es lo mismo exit() que pthread_exit()
	// si no ponemos nada, el proceso ejecuta implicitamente exit() y eso mata _todo; el proceso, los hilos, libera todo
	// pero con pthread_exit() el SO mantiene informacion del proceso y sus estructuras para que las usen los hilos hijos
	// eso permite que los hilos hijos sigan vivos hasta que terminen sus propia ejecucion
	//pthread_exit(NULL);
}
