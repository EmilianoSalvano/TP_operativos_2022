#include "../include/conexiones.h"
#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


int iniciar_server(char* ip, char* puerto) {
    int error = 0;
	int socket_servidor;
    struct addrinfo hints, *servinfo;

    // Inicializando hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    //hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe los addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    bool conecto = false;
    // Itera por cada addrinfo devuelto
    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        error = errno;

        if (socket_servidor < 0) {
        	continue;
        }

        // CANSADO DEL ERROR 98 - Address already in use ??
        // Esto es para que el servidor reuse el socket, aunque no nse haya complido el TIME-WAIT
        // con esto se evita el error 98 - Address already in use
        // pero todo lo que este conectado a ese socket previamente, va a ser conectado al nuevo binding
        // el resultado es indeterminado
        // si lo necesitan usar, lo descomentan. No es recomendable, el resultado deja de ser deterministico
        /*
        int yes = 1;
        if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0) {
        	error = errno;
        	close(socket_servidor);
        	fprintf(stderr, "setsockopt() failed. Error: %d\n", error);
        	return (error != 0 ? -abs(error) : -1);
        }
        */


        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) < 0) {
        	error = errno;
            close(socket_servidor);
            continue;
        }

        // Ni bien conecta uno nos vamos del for
        conecto = true;
        break;
    }

    if(!conecto) {
    	freeaddrinfo(servinfo);
        return (error != 0 ? -abs(error) : -1);
    }

    // Escuchando (hasta SOMAXCONN conexiones simultaneas)
    if (listen(socket_servidor, SOMAXCONN) < 0) {
    	error = errno;
    	freeaddrinfo(servinfo);

    	//if (socket_servidor >= 0)
		close(socket_servidor);

		return (error != 0 ? -abs(error) : -1);
    }

    freeaddrinfo(servinfo);
    return socket_servidor;
}



int esperar_cliente(int socket_servidor, char* ip_cliente) {
	struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
    int error = errno;

    if (socket_cliente < 0)
    	return (error !=0 ? -abs(error) : -1);

    ip_cliente = inet_ntoa(dir_cliente.sin_addr);

    return socket_cliente;
}




//int crear_conexion(t_log* logger, const char* server_name, char* ip, char* puerto) {
int crear_conexion(char* ip, char* puerto) {
    struct addrinfo hints, *servinfo;

    // Init de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;		// este no se que es???

    // Recibe addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    // Crea un socket con la informacion recibida (del primero, suficiente)
    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    int error = errno;

    // Fallo en crear el socket
    if(socket_cliente < 0) {
    	freeaddrinfo(servinfo);
    	return (error !=0 ? -abs(error) : -1);
    }

    // Error conectando
    if(connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    	error = errno;
        freeaddrinfo(servinfo);
        return (error !=0 ? -abs(error) : -1);
    }

    freeaddrinfo(servinfo);
    return socket_cliente;
}


/*
 * Errors:
	EBADF : fd isn't a valid open file descriptor.
	EINTR : The close() call was interrupted by a signal; see signal(7).
	EIO : An I/O error occurred.
 */
int liberar_conexion(int _socket) {
	shutdown(_socket, SHUT_RDWR);

	int result = close(_socket);
    int error = errno;

	if (result >= 0)
		return result;
	else
		return (error !=0 ? -abs(error) : -1);
}
