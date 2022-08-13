# TP_operativos_2022
Tp de operativos del grupo "Los Ñoños" del primer cuatrimestre del 2022

Algunos bugs:

-Las ip locales estan hardcodeadas, asi que para iniciar los SV hay que poner la IP de la maquina en la que se esta.

-Algunas IP de conexion tambien estan hardcodeadas, habria que poner para que tome el valor del archivo de configuracion.

-Hay un error en el kernel en el que, al entrar un proceso a ejecutar si este sale inmediatamente por una interrupcion se calcula igualmente la rafaga por mas que no se ejecute nada. Esto causa que la rafaga estimada del proceso sea muy corta, supuestamente es una boludez de arreglar (pero como no hice el kernel no lo hago).

-Si no existe la direccion de la ruta para crear los archivos SWAP de memoria, va a romper.



-En la carpeta config estan los archivos de configuracion (algunos estan medio como el orto y pueden causar seg fault), recordar modificar la IP.
-En carpeta-PRUEBAS estan los archivos de proceso que usa la consola.
-Hay 2 pdf para guiarse, uno de consigna y otro de pruebas.


Para compilar usar el comando "make", para limpiar "make clean", para ejecutar "./main.out parametro1 parametro2" el primer parametro siempre es el nivel de log (menos para la consola) este es un valor entre 1 y 4, 2 es nivel INFO (ver logs de las commons) y el se gundo parametro es el path al archivo de configuracion (quiza algun modulo requiere un tercer parametro, pero no creo).
