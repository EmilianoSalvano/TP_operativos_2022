# TP_operativos_2022
Tp de operativos del grupo "Los Ñoños" del primer cuatrimestre del 2022

Algunos bugs:

-Las ip locales estan hardcodeadas, asi que para iniciar los SV hay que poner la IP de la maquina en la que se esta.

-Algunas IP de conexion tambien estan hardcodeadas, habria que poner para que tome el valor del archivo de configuracion.

-Hay un error en el kernel en el que, al entrar un proceso a ejecutar si este sale inmediatamente por una interrupcion se calcula igualmente la rafaga
por mas que no se ejecute nada.
Esto causa que la rafaga estimada del proceso sea muy corta, supuestamente es una boludez de arreglar (pero como no hice el kernel no lo hago).

-Si no existe la direccion de la ruta para crear los archivos SWAP de memoria, va a romper.
