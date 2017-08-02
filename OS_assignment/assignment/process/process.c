#ifndef _SVID_SOURCE
	#define _SVID_SOURCE
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/ipc.h>

sem_t mutexLock;
int running_processes = 0;

/* Function used to validate the row */
void validRow(int row, int grid[9][9], int* buffer2, int* counter, pid_t parentId)
{
	FILE* f = fopen("log.txt", "a");
	
	int ii, validSubGrids = 0, numTo9;
	
	/*	Used to check each number in the row are valid 
	*	If along the way, a number is evaluated and at that number's slot in the checkArray
	*	is already marked as 1 (meaning there has been an occurrance of that number), then
	*	it will print an error and carry on with the next slot.
	*	If the number being evaluated against the checkArray marked as 0
	*	(meaning the occurence of that number has yet to appear), mark that slot as 1
	*	increment the total of validSubGrids by 1 and continue.
	*/
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	
	/* Validates that the row and column being evaluated is valid */
	if(row > 8 || row < 0)
	{
		printf("Invalid row: %d\n", row);
		exit(-1);
	}
	
	/* Checks at row indicated through parameter at the ii'th column to see if the number is valid. */
	for(ii = 0; ii < 9; ii++)
	{
		/* Gets the value at index (row, i) to evaluate */
		numTo9 = grid[row][ii];
		
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
				fprintf(f, "%d-%d: row %d is invalid\n", parentId + row, row+1, row+1);
			}
		}
		else
		{
			/* mark the slot as valid */
			checkArray[numTo9-1] = 1;
			validSubGrids++;
		}
	}
	
	/* Spinlocks until the semaphore is available to be used */
	while(running_processes == 1)
	{
		sem_wait(&mutexLock);
	}
	running_processes++;
	
	/* Alters shared memory */
	if(validSubGrids == 9)
	{
		*(buffer2+row) = 1;
		*counter = *counter + 1;
	}
	
	/* Unlocks lock for another process to use */
	running_processes--;
	sem_post(&mutexLock);
	
	fclose(f);
}

/* Function to evaluate each column for validity */
void validColumn(int grid[9][9], int* buffer2, int* counter, pid_t parentId)
{
	int ii, jj, kk, numTo9, isValid = 0;
	int validSubGrids = 0, validColumn = 0;
	FILE* f = fopen("log.txt", "a");
	
	/*	Used to check each number in the column are valid 
	*	If along the way, a number is evaluated and at that number's slot in the checkArray
	*	is already marked as 1 (meaning there has been an occurrance of that number), then
	*	it will print an error and carry on with the next slot.
	*	If the number being evaluated against the checkArray marked as 0
	*	(meaning the occurence of that number has yet to appear), mark that slot as 1
	*	increment the total of validSubGrids by 1 and continue.
	*/
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	
	/* Increments the column index each time all the rows are gone through at a particular column */
	for(jj = 0; jj < 9; jj++)
	{
		/* Checks at row indicated through parameter at the ii'th column to see if the number is valid. */
		for(ii = 0; ii < 9; ii++)
		{
			numTo9 = grid[ii][jj]; /* Gets the value at index (row, i) to evaluate */
			
			/* numTo9 signifies all the numbers available for sudoku 1-9. checkArray used numTo9 "-1" because it indexes at 0 */
			if(numTo9 < 1 || numTo9 > 9 || checkArray[numTo9 - 1] == 1)
			{
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
				fprintf(f, "%d-10: column %d is invalid\n", parentId + 9, jj+1);
			}
		}
		isValid = 0;
		validColumn = 0;
	}
	
	/* Spinlocks until lock is available */
	while(running_processes == 1)
	{
		sem_wait(&mutexLock);
	}
	running_processes++;
	
	/* Alters shared memory */
	*(buffer2+9) = validSubGrids;
	*counter = *counter + validSubGrids;
	
	/* Unlock for another process to use */
	running_processes--;
	sem_post(&mutexLock);
	fclose(f);
}

/* Function used to validate a 3x3 sub-grid */
void valid3x3(int grid[9][9], int* buffer2, int* counter, pid_t parentId)
{
	int row = 0, col = 0, valid3x3 = 0, numTo9 = 0, validSubGrids = 0, isValid = 0;
	int checkArray[9] = {0,0,0,0,0,0,0,0,0};
	FILE* f = fopen("log.txt", "a");
	int ii = 0, jj = 0, kk = 0;
	
	do
	{
		row = 0;
		for (ii = row; ii < 9; ii++)
		{
			for (jj = col; jj < col + 3; jj++)
			{
				numTo9 = grid[ii][jj];
				if (numTo9 < 1 || numTo9 > 9 || checkArray[numTo9 - 1] == 1)
				{
					/* Notify that there is an invalid subregion */
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
						fprintf(f, "%d-11: sub-grid [%d...%d %d...%d] is valid\n", parentId + 11, ii-1, ii+1, jj-1, jj+1);
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
				
				
				
				/* reset counters for valid subregion and valid 3x3 */
				isValid = 0;
				valid3x3 = 0;
			}
		}
		
		col = col + 3;
	}while(col != 9);
	
	/* Spinlocks until lock is available */
	while(running_processes == 1)
	{
		sem_wait(&mutexLock);
	}
	running_processes++;
	
	/* Alters shared memory */
	*(buffer2+10) = validSubGrids;
	*counter = *counter + validSubGrids;
	
	/* Unlocks mutex for another process to use */
	running_processes--;
	sem_post(&mutexLock);
	
	fclose(f);
}

/* Function to print the grid that contains the values of the sudoku puzzle */
void printPuzzle(int grid[9][9])
{
	int ii, jj;
	for(ii = 0; ii < 9; ii++)
	{
		for(jj = 0; jj < 9; jj++)
		{
		printf("%d ", grid[ii][jj]);
		}
		printf("\n");
	}
	printf("\n");
	
}

/* Function to access the buffer1 that contains the grid when the file is opened */
void readFile(int grid[9][9], char* buffer1)
{
	int ii, jj, lineCount = 0;

	/* Maps each grid index with the value given via the buffer */
	for(ii = 0; ii < 9; ii++)
	{
		for(jj = 0; jj < 9; jj++)
		{
			sscanf(buffer1 + lineCount, "%d", &grid[ii][jj]);
			lineCount = lineCount + 2;
		}
		
	}
}

/* Prints all the validation statements for the specifications with the values in buffer2 and counter with the process IDs */
void printValidation(int* buffer2, int* counter, pid_t parentId)
{
	int ii;
	printf("\n------------------------------------------------------\n");
	printf("COUNTER: %d\n", *counter);
	for(ii = 0; ii < 11; ii++)
	{
		if(ii < 9 && *(buffer2+ii) == 1)
		{
			printf("\nValidation result from %d-%d: row %d is valid", parentId + ii, ii+1, ii+1);
		}
		else if(ii < 9 && *(buffer2+ii) == 0)
		{
			printf("\nValidation result from %d-%d: row %d is invalid", parentId + ii, ii+1, ii+1);
		}
		else if(ii == 9)
		{
			printf("\nValidation result from %d-%d: %d of 9 columns are valid", parentId + ii, ii+1, *(buffer2+9));
		}
		else if(ii == 10)
		{
			printf("\nValidation result from %d-%d: %d of 9 sub-grids are valid", parentId + ii, ii+1, *(buffer2+10));
		}
		else
		{
			printf("\nERROR!!\n");
		}
	}
	
	if(*counter == 27)
	{
		printf("\n\nThere are 27 valid sub-grids, and thus the solution is valid.\n");
	}
	else
	{
		printf("\n\nThere are %d valid sub-grids, and thus the solution is invalid.", *counter);
	}
	printf("\n------------------------------------------------------\n");
}

int main(int argc, char* argv[])
{
	pid_t parentId, pid, childPid = 0;
	int grid[9][9];
	int sudokuPuzzle = 0, buf2 = 0, countPtr = 0;
	char* buffer1 = NULL;
	int* buf2Ptr;
	int* counterPtr;
	char* filename = argv[1];
	int PUZZLE_SIZE = 9*9*sizeof(int);
	int maxDelay = atoi(argv[2]);
	
	/* Resets all content in the log file to blank before executing the validations */
	FILE* f = fopen("log.txt", "w");
	fclose(f);
	
	/* Initialises the mutex to value of 1 to be able to be accessed by a process */
	sem_init(&mutexLock, 0, 1);
	
	/* READING IN THE SUDOKU PUZZLE FROM THE FILE AND ALLOCATING IT TO A GRID */
	sudokuPuzzle = open(filename, O_RDWR);
	
	if(sudokuPuzzle == -1)
	{
		printf("Error opening file\n");
	}
	
	buffer1 = mmap(0, PUZZLE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sudokuPuzzle, 0);
	
	readFile(grid, buffer1);
	printPuzzle(grid);
	
	/* Creates the memory space for buffer2 to identify the various subregions that are valid */
	buf2 = shm_open("buf2Ptr", O_CREAT|O_RDWR, 0666);
	buf2Ptr = mmap(0, 11*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, buf2, 0);
	
	/* Creates the memory space for counter to identify the amount of valid subgrids 
	 * Using MAP_ANONYMOUS intiialised the value of the mmap to 0 */
	countPtr = shm_open("counterPtr", O_CREAT|O_RDWR, 0666);
	counterPtr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, countPtr, 0);
	*counterPtr = 0;
	
	/* Forks child processes to allocate a region to validate */
	parentId = getpid();
	fork();
	pid = fork();
	fork();
	if(pid == 0)
	{
		fork();
	}
	childPid = getpid() - parentId;
	
	if(childPid == 0)
	{
		wait(NULL);
	}
	else if(childPid > 0 && childPid < 10)
	{
		/* DO ROW STUFF */
		validRow(childPid-1, grid, buf2Ptr, counterPtr, parentId);
		sleep(rand() % maxDelay);
		
	}
	else if (childPid == 10)
	{
		/* DO COLUMN STUFF */
		validColumn(grid, buf2Ptr, counterPtr, parentId);
		sleep(rand() % maxDelay);
		
	}
	else if (childPid == 11)
	{
		/* DO 3x3 STUFF */
		valid3x3(grid, buf2Ptr, counterPtr, parentId);
		sleep(rand() % maxDelay);
	}
	else
	{
		printf("ERROR IN FORKING PROCESSES -- PID: %d\n", childPid);
	}
	
	wait(NULL);
	wait(NULL);
	wait(NULL);
	
	/* When all child processes finish, print validation statements */
	if(childPid == 0)
	{
		printValidation(buf2Ptr, counterPtr, parentId);
	}
	
	shm_unlink("buf2Ptr");
	shm_unlink("counterPtr");
	munmap(buffer1, PUZZLE_SIZE);
	munmap(buf2Ptr, 11*sizeof(int));
	munmap(counterPtr, sizeof(int));
	
	exit(0);
	return 0;
}