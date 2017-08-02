void validRow(int row, int grid[9][9], int* buffer2, int* counter);
void validColumn(int grid[9][9], int* buffer2, int* counter);
void valid3x3(int grid[9][9], int* buffer2, int* counter);
void printPuzzle(int grid[9][9]);
void readFile(int grid[9][9], char* buffer1);
void printValidation(int* buffer2, int* counter, pid_t parentId);