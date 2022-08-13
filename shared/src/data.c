/*
 * data.c
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include "../include/data.h"


/************************* t_data *********************************************************/
t_data* data_create(void* _info) {
	t_data* self = malloc(sizeof(t_data));
	self->info = _info;
	self->next = NULL;
	return self;
}

void data_add(t_data** head, void* _info) {
	t_data* new = data_create(_info);

	if( head == NULL ) {
		head = &new;
	}
	else {
		t_data* aux = *head;

		while( aux->next != NULL){
			aux = aux->next;
		}

		aux->next = new;
	}
}

/*
void data_add(t_data** data, void* _info) {
	t_data *aux, *ant;

	aux = data;
	while (aux != NULL) {
		ant = aux;
		aux = ant->next;
	}

	if (ant == NULL) {
		data = data_create();
		data->info= _info;
		data->next = NULL;
	}
	else {
		ant->next = data_create();
		ant->next->info = _info;
		ant->next->next = NULL;
	}
}
*/

void data_destroy(t_data** head) {
	t_data* aux;

	while(*head != NULL){
		aux = *head;
		*head = (*head)->next;
		free(aux);
	}
}

/*
void data_destroy(t_data** data) {
	t_data *ant, *aux;

	aux = data;
	while (aux != NULL) {
		ant = aux;
		aux = ant->next;
		free(aux);
	}

	&data = NULL;
}
*/

void data_destroy_and_destroy_info(t_data** head, void(*destroy_info)(void*)) {
	t_data* aux;

	while(*head != NULL){
		aux = *head;
		*head = (*head)->next;
		destroy_info(aux->info);
		free(aux);
	}
}
