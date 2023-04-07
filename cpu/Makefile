#Los comentarios son las lineas que comienzan con "#"
#Defino variables:
#Compilador

CC = gcc


#Nombre del archivo objeto que compilo

OUTFILE = main.out

#SRC contiene los nombres de los archivos fuente de codigo (.c)
#En este caso quiero: encontrar los archivos de src (find ./src) de tipo file (-type f)
#que tengan el nombre algo.c (-iname "*.c"). Esto me da strings de la siguiente forma "./src/suma.c"
#para sacarle el "./src/" uso (-exec basename {} \;). Queda una cosa asi:
# " find ./src -type f -iname "*.c" -exec basename {} \;" esto podria usarlo en la terminal.
#Por ultimo, para usarlo aca, el shell indica que todo es un comando de shell:

SRC= $(shell find ./src -type f -iname "*.c" -exec basename {} \;)


#OBJ son los archivos objeto de la carpeta Include del modulo.
#Como (en una primera ejecucion) estos archivos aun no existen, lo que hago es
#tomar todos los strings ".c" que consegui en la variable SRC y cambiarles la extension a ".o" y 
#ademas agregarle el string de la ruta en la que estaran "./obj/". El % toma el valor
# de strings de cualquier longitud que esten antes del ".c":

OBJ = $(SRC:%.c=./obj/%.o)


#SHARED son los objetos de la carpeta shared (compartida entre modulos).

SHARED = $(shell find ../shared/obj/*.o)


#variable de librerias a linkear

LINK = -lpthread -lcommons -lm


#CFLAGS, con esta variable voy a elegir el modo de compilacion. Con esto voy a poder hacer debug despues.

CFLAGS = -c


#REGLAS

#La regla build se va a encargar de compilar al programa final, y cada que se ejecute va a ejecutar las
#reglas de las que dependen. La regla "makefolder" va ver si existe el directorio "dir" si no la crea,
#"makeshared" llama a otro archivo makefile con el cual se generan los archivos para "SHARED",
#luego "$(OBJ)" con la regla "./obj/%.o" va a ver si existe cada uno de los archivos ".o", y si alguno no 
#existe lo genera.
#Ahora, en lugar de poner "gcc -o main.o main.c dependencia1 dependencia2 dependenciaN", uso variables.

build: makefolder makeshared $(OBJ)
	$(CC) -o $(OUTFILE) main.c $(OBJ) $(SHARED) $(LINK)



#Para buscar leaks de memoria

valgrind: build
	valgrind --leak-check=full ./$(OUTFILE)


#Con esto puedo elegir como compila (con -c o con -g)

debug: CFLAGS := $(CFLAGS) -g
debug: makeshared_debug build


#la regla para el $(OBJ) es que para cualquier cosa de la forma "./obj/%.o" necesito su "./obj/%.c"
#luego, el gcc va a compilar esos ".c" para generar el ".o"

./obj/%.o: ./src/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LINK)


#Si el directorio "dir" no existe (! -d .7dir) entonces lo crea (then mkdir obj). El "if" se cierra con un "fi".
#NOTA: ver comando "make clean" para que recompile desde cero todos los obj.

makefolder:
	if [ ! -d ./obj ]; then mkdir obj; fi


#makeshared se encarga de llamar al makefile de la carpeta shared, para asegurarse de que este 
#cree los archivos objeto necesarios para compilar este modulo.

makeshared:
	make -C ../shared


makeshared_debug:
	make debug -C ../shared


#Clean borra cosas que ya no use aca y en shared

clean:
	rm -rf obj
	rm -rf *.out
	make clean -C ../shared
