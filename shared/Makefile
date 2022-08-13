#es una version con menos tareas del makefile de la carpeta del modulo

CC = gcc
SRC= $(shell find ./src -type f -iname "*.c" -exec basename {} \;)
OBJ = $(SRC:%.c=./obj/%.o)
LINK = -lpthread -lcommons -lm
CFLAGS = -c

build: makefolder $(OBJ)

debug: CFLAGS := $(CFLAGS) -g
debug: build

./obj/%.o: ./src/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LINK)

makefolder:
	if [ ! -d ./obj ]; then mkdir obj; fi

clean:
	rm -rf obj