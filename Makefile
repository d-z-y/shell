#Makefile
CFLAGS = -pthread -m64 -Wall
objects = main.c myshell.c job.c wrapped.c
all:
	@echo compiling $(objects)...
	gcc $(CFLAGS) $(objects) -o myshell
.PHONY: clean
clean:
	find . -type f -print0 | xargs -0 rm -f

