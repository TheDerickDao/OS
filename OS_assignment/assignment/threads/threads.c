#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include "threads.h"

volatile int running_threads = 0;
pthread_t threads[11];
pthread_mutex_t mutexLock;
pthread_cond_t condition;

/* This function is to read in the sudoku puzzle and allocate the grid to buffer1 */
void readFile(char* filename, Shared_Memory* shared_memory)
{
	FILE* f = fopen(filename, "r");
	int done = 0, readN, sudokuNum, row = 0, jj;
	
	if(f == NULL)
	{
		perror("Error in opening file for Sudoku Puzzle");
	}
	else
	{
		do
		{
			for(jj = 0; jj < 9; jj++)
			{
				/* read the variable in the file */
				readN = fscanf(f, "%d", &sudokuNum); 
				
				/* If it reads the end of file, stop the loop */
				if(readN == EOF) 
				{
					done = 1;
				}
				/* Otherwise, continue */
				else 
				{
					/* At a particular slot in row ii and column jj, place the number being read in into that slot */
					shared_memory->buffer1[row][jj] = sudokuNum;
				}
				/* Reads in the space that separates the numbers in the sudoku solution */
				readN = fscanf(f, " ");
			}
			row++;
		}while(!done);
	}
	fclose(f);
}

/* This function is to validate the rows to see if all numbers are unique */
void* validRow(void *sh_mem)
{
	Shared_Memory* shared_memory = (Shared_Memory*)sh_mem;
	
	/* defines its own address */
	pthread_t myTid = pthread_self();
	FILE* f = fopen("log.txt", "a");
	
	int ii, validSubGrids = 0, foundThread = 0, row = 0, numTo9;
	
	/*	Used to check each number in the row are valid 
	*	If along the way, a number is evaluated and at that number's slot in the checkArray
	*	is already marked as 1 (meaning there has been an occurrance of that number), then
	*	it will print an error and carry on with the next slot.
	*	If the number being evaluated against the checkArray marked as 0
	*	(meaning the occurence of that number has yet to appear), mark that slot as 1
	*	increment the total of validSubGrids by 1 and continue.
	*/
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	
	foundThread = pthread_equal(myTid, threads[row]);
	
	while((row < 8) && (foundThread == 0))
	{
		/* checks current thread against all the other threads incrementing each time it is wrong, to provide the row number */
		foundThread = pthread_equal(myTid, threads[row]);
		row++;
	}
	
	
	/* Validates that the row and column being evaluated is valid */
	if(row > 8 || row < 0)
	{
		printf("Invalid row: %d", row);
		pthread_exit(NULL);
	}
	
	/* Checks at row indicated through parameter at the ii'th column to see if the number is valid. */
	for(ii = 0; ii < 9; ii++)
	{
		/* Gets the value at index (row, i) to evaluate */
		numTo9 = shared_memory->buffer1[row][ii]; 
		
		/* numTo9 signifies all the numbers available for sudoku 1-9. checkArray used numTo9 "-1" because it indexes at 0 */
		if(numTo9 < 1 || numTo9 > 9 || checkArray[numTo9 - 1] == 1)
		{
			/* WRITE TO FILE */
			if(f == NULL)
			{
				perror("Error in opening file to write to.\n");
			}
			else
			{
				fprintf(f, "%lu-%d: row %d is invalid\n", myTid, row+1, row+1);
			}
		}
		else
		{
			/* mark the slot as valid */
			checkArray[numTo9-1] = 1;
			validSubGrids++;
		}
	}
	
	pthread_mutex_lock(&mutexLock);
	while(running_threads == 1)
	{
		pthread_cond_wait(&condition, &mutexLock);
	}
	running_threads++;
	
	/* Allocates certain values to the buffer2 and counter */
	if(validSubGrids == 9)
	{
		shared_memory->buffer2[row] = 1;
		shared_memory->counter++;
	}
	
	fclose(f);
	pthread_cond_signal(&condition);
	running_threads--;
	pthread_mutex_unlock(&mutexLock);
	pthread_exit(NULL);
}

void* validColumn(void* sh_mem)
{
	
	int ii, jj, kk, validSubGrids = 0, validColumn = 0, numTo9, isValid = 0;
	FILE* f = fopen("log.txt", "a");
	pthread_t myTid = pthread_self();
	
	/*	Used to check each number in the column are valid 
	*	If along the way, a number is evaluated and at that number's slot in the checkArray
	*	is already marked as 1 (meaning there has been an occurrance of that number), then
	*	it will print an error and carry on with the next slot.
	*	If the number being evaluated against the checkArray marked as 0
	*	(meaning the occurence of that number has yet to appear), mark that slot as 1
	*	increment the total of validSubGrids by 1 and continue.
	*/
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	Shared_Memory* shared_memory = (Shared_Memory*)sh_mem;
	
	
	
	/* Increments the column index each time all the rows are gone through at a particular column */
	for(jj = 0; jj < 9; jj++)
	{
		/* Checks at row indicated through parameter at the ii'th column to see if the number is valid. */
		for(ii = 0; ii < 9; ii++)
		{
			 /* Gets the value at index (row, i) to evaluate */
			numTo9 = shared_memory->buffer1[ii][jj];
			
			/* numTo9 signifies all the numbers available for sudoku 1-9. checkArray used numTo9 "-1" because it indexes at 0 */
			if(numTo9 < 1 || numTo9 > 9 || checkArray[numTo9 - 1] == 1)
			{
				/* WRITE TO FILE */
				isValid = 1;
			}
			else
			{
				/* mark the slot as valid */
				checkArray[numTo9-1] = 1;
				validColumn++;
			}
		}
		if(validColumn == 9)
		{
			validSubGrids++;
		}
		
		/* Used to reset the checkArray all to 0 */
		for(kk = 0; kk < 9; kk++)
		{
			checkArray[kk] = 0;
		}
		
		if(isValid == 1)
		{
			if(f == NULL)
			{
				perror("Error in opening file to write to.\n");
			}
			else
			{
				fprintf(f, "%lu-10: column %d is invalid\n", myTid, jj+1);
			}
		}
		isValid = 0;
		validColumn = 0;
	}
	
	fclose(f);
	
	/* Locks the mutex so this thread can access its critical section */
	pthread_mutex_lock(&mutexLock);
	while(running_threads == 1)
	{
		pthread_cond_wait(&condition, &mutexLock);
	}
	running_threads++;
	
	/* Update the buffer2 at slot [9] with the number of valid columns */
	shared_memory->buffer2[9] = validSubGrids;
	shared_memory->counter = shared_memory->counter + validSubGrids;
	
	/* Unlocks mutex for another thread to use the lock */
	pthread_cond_signal(&condition);
	running_threads--;
	pthread_mutex_unlock(&mutexLock);
	pthread_exit(NULL);
}

/* Function to evaluate a 3x3 sub-grid */
void* valid3x3(void* sh_mem)
{
	int row = 0, col = 0, valid3x3 = 0, numTo9 = 0, validSubGrids = 0, isValid = 0;
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	FILE* f = fopen("log.txt", "a");
	pthread_t myTid = pthread_self();
	Shared_Memory* shared_memory = (Shared_Memory*)sh_mem;
	
	int ii = 0, jj = 0, kk = 0;
	
	do
	{
		row = 0;
		for (ii = row; ii < 9; ii++)
		{
			for (jj = col; jj < col + 3; jj++)
			{
				numTo9 = shared_memory->buffer1[ii][jj];
				
				if (numTo9 < 1 || numTo9 > 9 || checkArray[numTo9 - 1] == 1)
				{
					isValid = 1;
				}
				else
				{
					checkArray[numTo9 - 1] = 1;
					valid3x3++;
				}
				
				/* If an invalid sub region is picked up, write to log file */
				if(isValid == 1 && (ii == 2 || ii == 5 || ii == 8) && (jj == 2 || jj == 5 || jj == 8))
				{
					if(f == NULL)
					{
						perror("Error in opening file to write to.\n");
					}
					else
					{
						fprintf(f, "%lu-11: sub-grid [%d...%d %d...%d] is valid\n", myTid, ii-1, ii+1, jj-1, jj+1);
					}
					isValid = 0;
				}
			}
			
			if(ii == 2 || ii == 5 || ii == 8)
			{
				/* Reset checkArray to 0 for next iteration */
				for(kk = 0; kk < 9; kk++)
				{
					checkArray[kk] = 0;
				}
				if(valid3x3 == 9)
				{
					validSubGrids++;
				}
				valid3x3 = 0;
			}
		}
		
		
		col = col + 3;
	}while(col != 9);
	
	fclose(f);
	
	/* Lock the mutex to change shared memory */
	pthread_mutex_lock(&mutexLock);
	while(running_threads == 1)
	{
		pthread_cond_wait(&condition, &mutexLock);
	}
	running_threads++;
	
	/* Update shared memory here */
	shared_memory->buffer2[10] = validSubGrids;
	shared_memory->counter = shared_memory->counter + validSubGrids;
	
	/* Unlock mutex for another thread to use */
	pthread_cond_signal(&condition);
	running_threads--;
	pthread_mutex_unlock(&mutexLock);
	pthread_exit(NULL);
}

/* Prints all the validation statements at the end */
void printValidation(Shared_Memory* shared_memory)
{
	int ii = 0;
	
	printf("COUNTER: %d\n", shared_memory->counter);
	for(ii = 0; ii < 11; ii++)
	{
		if(ii < 9 && shared_memory->buffer2[ii] == 1)
		{
			printf("\nValidation result from %lu-%d: row %d is valid", threads[ii], ii+1, ii+1);
		}
		else if(ii < 9 && shared_memory->buffer2[ii] == 0)
		{
			printf("\nValidation result from %lu-%d: row %d is invalid", threads[ii], ii+1, ii+1);
		}
		else if(ii == 9)
		{
			printf("\nValidation result from %lu-%d: %d of 9 columns are valid", threads[9], ii+1, shared_memory->buffer2[ii]);
		}
		else if(ii == 10)
		{
			printf("\nValidation result from %lu-%d: %d of 9 sub-grids are valid", threads[10], ii+1, shared_memory->buffer2[ii]);
		}
		else
		{
			printf("\nERROR!!\n");
		}
	}
	if(shared_memory->counter == 27)
	{
		printf("\n\nThere are 27 valid sub-grids, and thus the solution is valid.");
	}
	else
	{
		printf("\n\nThere are %d valid sub-grids, and thus the solution is invalid.", shared_memory->counter);
	}
}

/* Print puzzle that the program is working with */
void printPuzzle(Shared_Memory* shared_memory)
{
	int ii, jj;
	printf("\n");
	for(ii = 0; ii < 9; ii++)
	{
		for(jj = 0; jj < 9; jj ++)
		{
			printf("%d ", shared_memory->buffer1[ii][jj]);
		}
		printf("\n");
	}
}

int main(int argc, char* argv[])
{
	FILE* f;
	char* filename = NULL;
	int ii = 0, rc = 1, maxDelay;
	Shared_Memory* shared_memory = (Shared_Memory*)malloc(sizeof(Shared_Memory));
	filename = argv[1];
	maxDelay = atoi(argv[2]);
	
	/* Used to reset the log file each time the program starts running */
	f = fopen("log.txt", "w");
	fclose(f);
	
	/* Reads the sudoku puzzle from file */
	readFile(filename, shared_memory);
	printPuzzle(shared_memory);
	
	for(ii = 0; ii < 11; ii++)
	{
		shared_memory->buffer2[ii] = 0;
	}
	shared_memory->counter = 0;
	
	/* initialises the mutex for use */
	pthread_mutex_init(&mutexLock, NULL);
	
	/* initialises the condition to wait and signal */
	pthread_cond_init(&condition, NULL);
	
	/* Allocate each thread its section to validate */
	for(ii = 0; ii < 11; ii++)
	{
		if(ii < 9)
		{
			rc = pthread_create(&threads[ii], NULL, &validRow, (void*)shared_memory);
			sleep(rand() % maxDelay);
		}
		else if(ii == 9)
		{
			rc = pthread_create(&threads[9], NULL, &validColumn, (void*)shared_memory);
			sleep(rand() % maxDelay);
		}
		else if(ii == 10)
		{
			rc = pthread_create(&threads[10], NULL, &valid3x3, (void*)shared_memory);
			sleep(rand() % maxDelay);
			
		}
		else
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	
	for(ii = 0; ii < 11; ii++)
	{
		pthread_join(threads[ii], NULL);
	}
	
	/* Prints all the validation statements */
	printf("\n------------------------------------------------------\n");
	printValidation(shared_memory);
	printf("\n------------------------------------------------------\n");
	
	free(shared_memory);
	
	/* destroys the mutex after use */
	pthread_mutex_destroy(&mutexLock);
	pthread_exit(NULL);
}