typedef struct{
	int buffer1[9][9];
	int buffer2[11];
	int counter;
} Shared_Memory;

void readFile(char* filename, Shared_Memory* shared_memory);
void printPuzzle(Shared_Memory* shared_memory);
void* validRow(void* sh_mem);
void* validColumn(void* sh_mem);
void* valid3x3(void* shared_memory);
void printValidation(Shared_Memory* shared_memory);