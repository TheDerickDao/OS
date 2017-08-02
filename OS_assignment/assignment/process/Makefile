CC = gcc
CFLAGS = -Wall -ansi -pedantic -g
OBJ = process.o
EXEC = mssv

$(EXEC): $(OBJ)
	$(CC) -g $(OBJ) -o $(EXEC) -lpthread -lrt

process.o: process.c process.h
	$(CC) -c process.c $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)