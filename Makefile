# Definición del compilador y las banderas
CC = gcc
CFLAGS =

# Objetivo principal: compilar el programa
a.out: Shell_project.c job_control.c builtin_commands.c
	$(CC) $(CFLAGS) -o a.out Shell_project.c job_control.c builtin_commands.c

# Regla de limpieza
clean:
	rm -f a.out

# Para facilitar la depuración
.PHONY: all clean

