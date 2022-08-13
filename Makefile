#Este makefile se encarga de ejecutar reglas con los makefile de las carpetas listadas.
#NO es necesario hacer operaciones en la carpeta shared, porque ya es referenciada por las demas.

#all (en consola "make all") ejecuta el comando "make" en todas las carpetas listadas.

all:
	make -C consola
	make -C cpu
	make -C kernel
	make -C memoriaswap


#clean (en consola "make clean")  hace un "make clean" en todas las carpetas listadas.

clean:
	make clean -C consola
	make clean -C cpu
	make clean -C kernel
	make clean -C memoriaswap