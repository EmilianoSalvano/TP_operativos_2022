#ifndef CONEXIONES_H_
#define CONEXIONES_H_

/*
 *
 */
int iniciar_server(char* ip, char* puerto);

/*
 *
 */
int esperar_cliente(int socket_servidor, char* ip_cliente);

/*
 *
 */
int crear_conexion(char* ip, char* puerto);

/*
 *
 */
int liberar_conexion(int _socket);

#endif
