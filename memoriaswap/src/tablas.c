#include "../include/tablas.h"

List* lista_tablas_de_paginas;
List* lista_tablas_de_tablas;
t_log* logger_memoria;

//############Inicializar proceso
/*Crear nueva tabla de tablas paginas con cantidad necesaria de tablas de paginas segun el tamaño del proceso.
Si el proceso es muy grande tiene que dar error.

1) t_tabla_primer_nivel* nuevaTablaDeTablas = malloc(sizeof(t_tabla_primer_nivel));
2) generarTablasSegundoNivel(nuevaTablaDeTablas, tamañoProceso);		//esto genera x cantidad de tablas de paginas con los frames que necesite segun el tamaño del proceso.
																		//despues tiene que encolarla en la lista de tablas de segundo nivel.
3) encolar(nuevaTablaDeTablas, nuevaTablaDeTablas);
*/
//###########

//##########Crear tablas de segundo nivel
/*
1) En funcion de la cantidad de paginas que falte direccionar es la cantidad de elementos en la lista de la tabla de paginas, como maximo la cantidad
de entradas maxima del archivo de configuracion_memoria.
2) se encola la tabla a la lista de tablas de pagina.
*/
//############



uint32_t numero_de_tabla(uint32_t numero) {
    return (numero / configuracion_memoria->entradas_por_tabla);
}

uint32_t numero_de_pagina(uint32_t numero){
    return (numero % configuracion_memoria->entradas_por_tabla);
}

void aplicar_retardo_swap(){
    usleep(1000 * configuracion_memoria->retardo_memoria);
}


uint32_t crear_tabla_primer_nivel(uint32_t tam_proceso, uint32_t id_proceso){

	int tam_maximo = (configuracion_memoria->tam_pagina * configuracion_memoria->entradas_por_tabla * configuracion_memoria->entradas_por_tabla);

    if(tam_proceso > tam_maximo) {
        log_error(logger_memoria, "el tamaño:[%d] de proceso supera al maximo:[%d] posible", tam_proceso, tam_maximo);
        return -1;        //El proceso es demaciado grande para ser direccionado
    }

    t_tabla_primer_nivel* nuevaTablaDeTablas = malloc(sizeof(t_tabla_primer_nivel));

    nuevaTablaDeTablas->puntero_marcos = 0;

    nuevaTablaDeTablas->pcb_id = id_proceso;

    nuevaTablaDeTablas->paginas_asignadas = malloc(sizeof(List));
    initlist(nuevaTablaDeTablas->paginas_asignadas);

    nuevaTablaDeTablas->tablas_segundo_nivel = malloc(sizeof(List));
    initlist(nuevaTablaDeTablas->tablas_segundo_nivel);

    //La tabla de tablas de paginas tiene:
    //cantidadDePaginas = tamañoProceso/tamañopagina
    uint32_t cantidadDePaginas = (uint32_t)ceil((float)tam_proceso/configuracion_memoria->tam_pagina);
    //cantidadDeTablasDePagina = cantidadDePaginas/entradasPorTabla
    uint32_t cantidadDeTablasDePagina = (uint32_t)ceil((float)cantidadDePaginas/configuracion_memoria->entradas_por_tabla);

    //printf("paginas necesarias: %d\n", cantidadDePaginas);
    //printf("tablas necesarias: %d\n", cantidadDeTablasDePagina);

    /*
    char string_log[200];
    sprintf(string_log, "Inicia_Proceso %d\tTablas_de_pagina:%d\tPaginas:%d", id_proceso, cantidadDeTablasDePagina, cantidadDePaginas);
    log_info(logger_memoria, string_log);
    */
    log_info(logger_memoria, "espacio de memoria reservado para pid:[%d] tablas de paginas[%d] paginas:[%d]", id_proceso, cantidadDeTablasDePagina, cantidadDePaginas);


    do{
        if(cantidadDeTablasDePagina < 1){
            crear_tabla_paginas(nuevaTablaDeTablas->tablas_segundo_nivel, configuracion_memoria->entradas_por_tabla);
        }else{
            crear_tabla_paginas(nuevaTablaDeTablas->tablas_segundo_nivel, cantidadDePaginas);
        }
    }while(--cantidadDeTablasDePagina > 0);     //primero decrementa despues evalua.

    pushbacklist(lista_tablas_de_tablas, nuevaTablaDeTablas);
    
	return (uint32_t)(sizelist(*lista_tablas_de_tablas)-1);
}

void crear_tabla_paginas(List* lista, uint32_t cantidadPaginas){

    t_tabla_segundo_nivel* nuevaTablaDePaginas = malloc(sizeof(t_tabla_segundo_nivel));
    nuevaTablaDePaginas->lista_paginas = malloc(sizeof(List));
    initlist(nuevaTablaDePaginas->lista_paginas);

    pushbacklist(lista_tablas_de_paginas, nuevaTablaDePaginas);             //Agrego una pagina a la lista de tablas de paginas.
    uint32_t* tam_lista_paginas = malloc(sizeof(uint32_t));
    *tam_lista_paginas = sizelist(*lista_tablas_de_paginas)-1;
    pushbacklist(lista, tam_lista_paginas);                                 //Agrego la direccion a la tabla de tablas de paginas del proceso.

    do{
        t_pagina* nuevaPagina = malloc(sizeof(t_pagina));
        nuevaPagina->numero_marco= 0;
        nuevaPagina->presencia = false;
        nuevaPagina->modificado = false;
        nuevaPagina->uso = false;

        pushbacklist(nuevaTablaDePaginas->lista_paginas, nuevaPagina);
    }while(--cantidadPaginas > 0);
}

uint32_t buscar_marco_libre(){
    uint32_t cantidad = configuracion_memoria->tam_memoria/configuracion_memoria->tam_pagina;

	for(uint32_t i = 0; i<cantidad; i++){
		if((registro_espacioUsuario + i)->asignado == false){
            (registro_espacioUsuario + i)->asignado = true;
			return i;
		}
	}
    return (-1);
}





uint32_t escribir_marco(uint32_t numero_marco, uint32_t desplazamiento, uint32_t dato) {
	log_debug(logger_memoria, "escribir_marco :: offset:[%d]", (numero_marco * (configuracion_memoria->tam_pagina)) + desplazamiento);
    memcpy(espacioUsuario + (numero_marco * (configuracion_memoria->tam_pagina)) + desplazamiento, &dato, sizeof(uint32_t));

    return 1;           //Dice que devuelve un OK si escribe el mensaje, yo supongo que tengo que validar que se escriba bien, o que no sea muy largo.
}

void leer_marco(uint32_t numero_marco, uint32_t desplazamiento, uint32_t* dato){
	log_debug(logger_memoria, "leer_marco :: offset:[%d]", (numero_marco * (configuracion_memoria->tam_pagina)) + desplazamiento);
    memcpy(dato, espacioUsuario + (numero_marco * (configuracion_memoria->tam_pagina)) + desplazamiento, sizeof(uint32_t));
}

uint32_t escribir_marco_completo(uint32_t numero_marco, void* dato){
    memcpy(espacioUsuario + (numero_marco * (configuracion_memoria->tam_pagina)), dato, configuracion_memoria->tam_pagina);
    return 1;           //Dice que devuelve un OK si escribe el mensaje, yo supongo que tengo que validar que se escriba bien, o que no sea muy largo.
}

void leer_marco_completo(uint32_t numero_marco, void* dato){
    memcpy(dato, espacioUsuario + (numero_marco * configuracion_memoria->tam_pagina), configuracion_memoria->tam_pagina);
}


//SWAP
//"Con memoria virtual, el disco es el limite..." -Esteban Masoero

char* numero_tabla_a_path(uint32_t numero_tabla_tablas){
	log_debug(logger_memoria, "numero_tabla_a_path :: numero_tabla_tablas:[%d]", numero_tabla_tablas);

    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);

    log_debug(logger_memoria, "numero_tabla_a_path :: tabla_tablas->pcb_id:[%d]", tabla_tablas->pcb_id);

    char* path = string_new();
    char* snumero_tabla = string_itoa((int)tabla_tablas->pcb_id);
    string_append(&path, configuracion_memoria->path_swap);
    string_append(&path, "/");
    string_append(&path, snumero_tabla);
    string_append(&path, ".swap");

    log_debug(logger_memoria, "numero_tabla_a_path :: numero_tabla_tablas:[%d] pid:[%d] path:[%s]", numero_tabla_tablas, tabla_tablas->pcb_id, path);

    free(snumero_tabla);
    return path;
}

void crear_archivo_swap(uint32_t numero_tabla_tablas, uint32_t tam_proceso){
    sem_wait(&semaforo);

        char* snumero_tabla = numero_tabla_a_path(numero_tabla_tablas);

        // TODO: esto pincha.. dejo una idea para crear un archivo vacio
        // https://ourcodeworld.com/articles/read/1280/how-to-create-empty-files-of-a-certain-size-in-linux

        void* relleno = malloc(configuracion_memoria->tam_pagina);          //Algo para escribir en el archivo.

        FILE* fp;
        fp = fopen(snumero_tabla, "wb");

        if(fp==NULL)
        {
            log_error(logger_memoria, "error de apertura de archivo swap:[%s]", snumero_tabla);
            exit(1);
        }

        fclose(fp);

        int bloques = ceil((double)tam_proceso / (double)configuracion_memoria->tam_pagina);
        int file_size = bloques * configuracion_memoria->tam_pagina;
        truncate(snumero_tabla, file_size);

        /*
        for(int i = 0; i<(tam_proceso/configuracion_memoria->tam_pagina); i++){             //Estoy hay que revisarlo, a lo mejor la cantidad da mal
            fwrite(&relleno, configuracion_memoria->tam_pagina, 1, fp);
        }
        fclose(fp);
        */

        free(relleno);
        free(snumero_tabla);

        aplicar_retardo_swap();
    
    sem_post(&semaforo);
}

void* leer_pagina_swap(uint32_t numero_tabla_tablas, uint32_t numpero_tabla_paginas, uint32_t numero_pagina) {
    sem_wait(&semaforo);

    	log_debug(logger_memoria, "leer_pagina_swap :: numero_tabla_tablas:[%d], numpero_tabla_paginas:[%d], numero_pagina:[%d]",
    			numero_tabla_tablas, numpero_tabla_paginas, numero_pagina);

        void* dato = malloc(configuracion_memoria->tam_pagina);
        char* snumero_tabla = numero_tabla_a_path(numero_tabla_tablas);

        log_debug(logger_memoria, "leer_pagina_swap :: archivo:[%s] len:[%d] posicion:[%d]", snumero_tabla, 0,
                		((numpero_tabla_paginas * configuracion_memoria->entradas_por_tabla) + numero_pagina) * configuracion_memoria->tam_pagina);

        FILE* fp;
        fp = fopen(snumero_tabla,"rb");

        // no borras hasta terminmar las pruebas
        //(Las paginas de tablas anteriores + el desplazamiento de la tabla a la que entra) * el tamaño de las paginas.
        fseek(fp, ((numpero_tabla_paginas * configuracion_memoria->entradas_por_tabla) + numero_pagina) * configuracion_memoria->tam_pagina, SEEK_SET);

        fread(dato, configuracion_memoria->tam_pagina, 1, fp);

        fclose(fp);
        free(snumero_tabla);

        aplicar_retardo_swap();

    sem_post(&semaforo);

        return dato;
}

void escribir_pagina_swap(uint32_t numero_tabla_tablas, uint32_t numpero_tabla_paginas, uint32_t numero_pagina, void* dato){
    sem_wait(&semaforo);

        char* snumero_tabla = numero_tabla_a_path(numero_tabla_tablas);

        FILE* fp;
        fp = fopen(snumero_tabla, "r+b");

        if(fp==NULL)
        {
            log_error(logger_memoria, "error de escritura en archivo swap:[%s]", snumero_tabla);
            exit(1);
        }

        // (Las paginas de tablas anteriores + el desplazamiento de la tabla a la que entra) * el tamaño de las paginas.
        fseek(fp, ((numpero_tabla_paginas * configuracion_memoria->entradas_por_tabla) + numero_pagina) * configuracion_memoria->tam_pagina , SEEK_SET);

        fwrite(dato, configuracion_memoria->tam_pagina, 1, fp);

        fclose(fp);
        free(snumero_tabla);

        aplicar_retardo_swap();

    sem_post(&semaforo);
}

void eliminar_archivo_swap(uint32_t numero_tabla_tablas){
    sem_wait(&semaforo);

        char* snumero_tabla = numero_tabla_a_path(numero_tabla_tablas);
        remove(snumero_tabla);
        free(snumero_tabla);

        aplicar_retardo_swap();

    sem_post(&semaforo);
}


void* seleccionar_pagina_victima_clock(uint32_t numero_tabla_tablas){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    uint32_t* puntero = &(tabla_tablas->puntero_marcos); //Modificar al puntero modifica al puntero_marcos
    List* paginas_asignadas = tabla_tablas->paginas_asignadas;
    while(true){
        t_registros_asignada* aux = atlist(*paginas_asignadas, *puntero);
        if(!aux->pagina_asignada->uso){
            //Aux es un t_registros_asignada, por lo que contiene el numero de pagina y la pagina.
            return aux;
        }
        aux->pagina_asignada->uso = false;
        (*puntero) ++;
        if(*puntero == configuracion_memoria->marcos_por_proceso)
        {
            *puntero = 0;
        }
    }
}



void* seleccionar_pagina_victima_clock_M(uint32_t numero_tabla_tablas){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    uint32_t* puntero = &(tabla_tablas->puntero_marcos);
    List* paginas_asignadas = tabla_tablas->paginas_asignadas;

    while(true)
    {

        for(uint32_t i = 0; i<configuracion_memoria->marcos_por_proceso; i++){
            t_registros_asignada* aux = atlist(*paginas_asignadas, *puntero);
            if(!aux->pagina_asignada->uso && !aux->pagina_asignada->modificado){
                //Aux es un t_registros_asignada, por lo que contiene el numero de pagina y la pagina.

                return aux;
            }
            (*puntero) ++;
            if(*puntero == configuracion_memoria->marcos_por_proceso)
            {
                *puntero = 0;
            }
        }
        for(uint32_t i = 0; i<configuracion_memoria->marcos_por_proceso; i++){
            t_registros_asignada* aux = atlist(*paginas_asignadas, *puntero);
            if(!aux->pagina_asignada->uso && aux->pagina_asignada->modificado){

                return aux;
            }

            aux->pagina_asignada->uso = false;
            (*puntero) ++;
            if(*puntero == configuracion_memoria->marcos_por_proceso)
            {
                *puntero = 0;
            }
        }
    }
}

void* buscar_pagina_en_tablas(uint32_t numero_tabla_tablas, uint32_t numero_pagina){
    t_tabla_primer_nivel* tabla_de_tablas = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);

    uint32_t* numero_tabla_paginas = atlist(*tabla_de_tablas->tablas_segundo_nivel, numero_de_tabla(numero_pagina));

    t_tabla_segundo_nivel* tabla_de_paginas = atlist(*lista_tablas_de_paginas, *numero_tabla_paginas);

    t_pagina* pagina = atlist(*tabla_de_paginas->lista_paginas, numero_de_pagina(numero_pagina));

    return pagina;
}


//Si ya estan asignados el maximo de marcos del proceso.
void reemplazar_pagina_de_swap_a_memoria(uint32_t numero_tabla_tablas, uint32_t pagina){
    t_registros_asignada* victima;
    if(!strcmp(configuracion_memoria->algoritmo_reemplazo, "CLOCK")){
        victima = seleccionar_pagina_victima_clock(numero_tabla_tablas);
    }else if(!strcmp(configuracion_memoria->algoritmo_reemplazo, "CLOCK-M")){
        victima = seleccionar_pagina_victima_clock_M(numero_tabla_tablas);
    }else{
        log_error(logger_memoria, "Algoritmo:[%s] de reemplazo desconocido", configuracion_memoria->algoritmo_reemplazo);
        exit(1);
    }

    uint32_t numero_de_marco = victima->pagina_asignada->numero_marco;

    if(victima->pagina_asignada->modificado){
        uint32_t tabla_paginas = numero_de_tabla(victima->numero_pagina);
        void* pagina_de_marco = malloc(configuracion_memoria->tam_pagina);
        leer_marco_completo(numero_de_marco, pagina_de_marco);
        escribir_pagina_swap(numero_tabla_tablas, tabla_paginas, victima->numero_pagina, pagina_de_marco);
        victima->pagina_asignada->modificado = false;
        free(pagina_de_marco);
    }

    victima->pagina_asignada->presencia = false;

    void* pagina_de_swap = leer_pagina_swap(numero_tabla_tablas, numero_de_tabla(pagina), numero_de_pagina(pagina));
    escribir_marco_completo(victima->pagina_asignada->numero_marco, pagina_de_swap);
    free(pagina_de_swap);


    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    (tabla_tablas->puntero_marcos)++;
    if (tabla_tablas->puntero_marcos == configuracion_memoria->marcos_por_proceso)
    	tabla_tablas->puntero_marcos = 0;


    log_info(logger_memoria, "pagina replazada:[%d] por pagina:[%d] del proceso:[%d] en marco:[%d]", victima->numero_pagina, pagina, tabla_tablas->pcb_id, numero_de_marco);


    victima->numero_pagina = pagina;
    victima->pagina_asignada = buscar_pagina_en_tablas(numero_tabla_tablas, pagina);
    victima->pagina_asignada->presencia = true;
    victima->pagina_asignada->uso = true;
    victima->pagina_asignada->numero_marco = numero_de_marco;
    (registro_espacioUsuario + numero_de_marco)->pagina = pagina;
    (registro_espacioUsuario + numero_de_marco)->tabla_tablas = numero_tabla_tablas;
}


//Si aun quedan marcos del proceso para asignar.
void asignar_marco_y_cargar_pagina_de_swap(uint32_t numero_tabla_tablas, uint32_t pagina){
    uint32_t numero_de_marco = buscar_marco_libre();

    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);

    List* paginas_asignadas = tabla_tablas->paginas_asignadas;


    (tabla_tablas->puntero_marcos)++;
    if (tabla_tablas->puntero_marcos == configuracion_memoria->marcos_por_proceso)
    	tabla_tablas->puntero_marcos = 0;
    
    t_registros_asignada* nuevo_registro = malloc(sizeof(t_registros_asignada));
    nuevo_registro->pagina_asignada = buscar_pagina_en_tablas(numero_tabla_tablas, pagina);
    nuevo_registro->numero_pagina = pagina;
    nuevo_registro->pagina_asignada->uso = true;
    nuevo_registro->pagina_asignada->presencia=true;
    nuevo_registro->pagina_asignada->numero_marco = numero_de_marco;
    (registro_espacioUsuario + numero_de_marco)->pagina = pagina;
    (registro_espacioUsuario + numero_de_marco)->tabla_tablas = numero_tabla_tablas;

    pushbacklist(paginas_asignadas, nuevo_registro);

    void* pagina_de_swap = leer_pagina_swap(numero_tabla_tablas, numero_de_tabla(pagina), numero_de_pagina(pagina));
    escribir_marco_completo(numero_de_marco, pagina_de_swap);
    free(pagina_de_swap);

    log_info(logger_memoria, "pagina:[%d] cargada para proceso:[%d] en marco:[%d]", pagina, tabla_tablas->pcb_id, numero_de_marco);
}


//A la hora de cargar una pagina que no esta presente en memoria se usa este metodo:
void cargar_pagina_de_swap_a_memoria(uint32_t numero_tabla_tablas, uint32_t pagina){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);

    if(sizelist(*tabla_tablas->paginas_asignadas) < configuracion_memoria->marcos_por_proceso){
        asignar_marco_y_cargar_pagina_de_swap(numero_tabla_tablas, pagina);
    }else{
        reemplazar_pagina_de_swap_a_memoria(numero_tabla_tablas, pagina);
    }
}



// TODO: esta funcion devuelve siempre 1. deberia devolver, por convencion, 0 (EXIT_SUCCESS) o 1 (EXIT_FAILURE)
uint32_t escribir_en_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina, uint32_t desplazamiento, uint32_t dato){
    t_pagina* pagina = buscar_pagina_en_tablas(numero_tabla_tablas, numero_pagina);

    if(desplazamiento < configuracion_memoria->tam_pagina)
    {
        if (pagina->presencia){
        	log_info(logger_memoria, "escritura en pagina:[%d] desplazamiento:[%d] marco:[%d] dato:[%d]", numero_pagina, desplazamiento, pagina->numero_marco, dato);

            escribir_marco(pagina->numero_marco, desplazamiento, dato);
            pagina->uso = true;
            pagina->modificado = true;
        }else{
        	log_info(logger_memoria, "Page fault :: numero_tabla_tablas:[%d] numero_pagina:[%d]", numero_tabla_tablas, numero_pagina);

            cargar_pagina_de_swap_a_memoria(numero_tabla_tablas, numero_pagina);

            escribir_marco(pagina->numero_marco, desplazamiento, dato);
            pagina->modificado = true;
        }
    }
    else {
    	log_error(logger_memoria, "error de direccionamiento, el desplazamiento es mayor al tamaño de pagina. numero_tabla_tablas:[%d] numero_pagina:[%d] desplazamiento:[%d] tamaño de pagina:[%d] dato:[%d]",
    	        numero_tabla_tablas, numero_pagina, desplazamiento, configuracion_memoria->tam_pagina, dato);

    	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

uint32_t leer_en_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina, uint32_t desplazamiento){
    t_pagina* pagina = buscar_pagina_en_tablas(numero_tabla_tablas, numero_pagina);
    uint32_t dato = -1;

    //desplazamiento = desplazamiento * sizeof(uint32_t);
    if(desplazamiento >= configuracion_memoria->tam_pagina)
    {
        log_error(logger_memoria, "error de direccionamiento, el desplazamiento es mayor al tamaño de pagina. numero_tabla_tablas:[%d] numero_pagina:[%d] desplazamiento:[%d] tamaño de pagina:[%d]",
        		numero_tabla_tablas, numero_pagina, desplazamiento, configuracion_memoria->tam_pagina);

        return dato;
    }

    if(pagina->presencia){
        leer_marco(pagina->numero_marco, desplazamiento, &dato);
        pagina->uso = 1;

        log_info(logger_memoria, "escritura en pagina:[%d] desplazamiento:[%d] marco:[%d] dato:[%d]", numero_pagina, desplazamiento, pagina->numero_marco, dato);
    }else{
    	log_info(logger_memoria, "Page fault :: numero_tabla_tablas:[%d] numero_pagina:[%d]", numero_tabla_tablas, numero_pagina);

        cargar_pagina_de_swap_a_memoria(numero_tabla_tablas, numero_pagina);

        leer_marco(pagina->numero_marco, desplazamiento, &dato);
    }


    log_debug(logger_memoria, "leer_en_pagina-resultado :: numero_tabla_tablas:[%d] numero_pagina:[%d] desplazamiento:[%d] dato:[%d]",
    		numero_tabla_tablas, numero_pagina, desplazamiento, dato);

    return dato;
}

uint32_t buscar_marco_de_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina){
    t_pagina* pagina = buscar_pagina_en_tablas(numero_tabla_tablas, numero_pagina);
    if(!pagina->presencia){
        cargar_pagina_de_swap_a_memoria(numero_tabla_tablas, numero_pagina);
    }
    return pagina->numero_marco;
}

bool suspender_proceso(uint32_t numero_tabla_tablas){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    tabla_tablas->puntero_marcos = 0;
    if(tabla_tablas != NULL){
        while(sizelist(*tabla_tablas->paginas_asignadas)){
            t_registros_asignada* registro = popfrontlist(tabla_tablas->paginas_asignadas);
            void* pagina_de_marco = malloc(configuracion_memoria->tam_pagina);
            leer_marco_completo(registro->pagina_asignada->numero_marco, pagina_de_marco);
            escribir_pagina_swap(numero_tabla_tablas, numero_de_tabla(registro->numero_pagina), registro->numero_pagina, pagina_de_marco);
            registro->pagina_asignada->presencia = false;
            registro->pagina_asignada->uso = false;
            registro->pagina_asignada->modificado = false;
            (registro_espacioUsuario + registro->pagina_asignada->numero_marco)->asignado = false;
            free(pagina_de_marco);
            free(registro);
        }

        /*
        char string_log[200];
        sprintf(string_log, "Suspendido el proceso %d", tabla_tablas->pcb_id);
        log_info(logger_memoria, string_log);
        */

        log_info(logger_memoria, "proceso:[%d] suspendido", tabla_tablas->pcb_id);


        return true;
    }
    else{
        //El proceso fue finalizado o no existe
        return false;
    }
}

bool finalizar_proceso(uint32_t numero_tabla_tablas){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    if(tabla_tablas != NULL)
    {
        while(sizelist(*tabla_tablas->paginas_asignadas)){
            t_registros_asignada* registro = popfrontlist(tabla_tablas->paginas_asignadas);
            (registro_espacioUsuario + registro->pagina_asignada->numero_marco)->asignado = false;
            eliminar_archivo_swap(numero_tabla_tablas);
            free(registro);
        }

        /*
        char string_log[200];
        sprintf(string_log, "Finalizado el proceso %d", tabla_tablas->pcb_id);
        log_info(logger_memoria, string_log);
		*/

        log_info(logger_memoria, "proceso:[%d] finalizado", tabla_tablas->pcb_id);

        //borrar_tablas_proceso(numero_tabla_tablas);

        return true;
    }
    else
    {
        //Este proceso ya fue finalizado o no existe
        return false;
    }
}

uint32_t iniciar_nuevo_proceso(uint32_t tam_proceso, uint32_t id_proceso){
    uint32_t numero_proceso = crear_tabla_primer_nivel(tam_proceso, id_proceso);
    crear_archivo_swap(numero_proceso, tam_proceso);
    return numero_proceso;
}


void borrar_tablas_proceso(uint32_t numero_tabla_tablas){
    t_tabla_primer_nivel* tabla_tablas  = atlist(*lista_tablas_de_tablas, numero_tabla_tablas);
    while(sizelist(*tabla_tablas->tablas_segundo_nivel)){

        uint32_t* numero_tabla_paginas = popfrontlist(tabla_tablas->tablas_segundo_nivel);

        t_tabla_segundo_nivel* tabla_de_paginas = atlist(*lista_tablas_de_paginas, *numero_tabla_paginas);

        while(sizelist(*tabla_de_paginas->lista_paginas)){
            t_pagina* pagina = popfrontlist(tabla_de_paginas->lista_paginas);
            free(pagina);
        }
        free(tabla_de_paginas);
        free(numero_tabla_paginas);
    }

    while(sizelist(*tabla_tablas->paginas_asignadas)){
        t_registros_asignada* registro = popfrontlist(tabla_tablas->paginas_asignadas);
        free(registro);
    }

    free(tabla_tablas);
    tabla_tablas = NULL;
}
