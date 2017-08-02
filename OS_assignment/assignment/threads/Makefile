CC = gcc
CFLAGS = -Wall -ansi -pedantic -g
OBJ = threads.o
EXEC = mssv

$(EXEC): $(OBJ)
	$(CC) -pthread -g $(OBJ) -o $(EXEC)

threads.o: threads.c threads.h
	$(CC) -c threads.c $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)