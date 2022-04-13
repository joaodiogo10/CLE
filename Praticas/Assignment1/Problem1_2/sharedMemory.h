#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "probConst.h"

#define SUCCESS 1
#define FAILURE 0

struct sFileHandler;
typedef struct sFileHandler *FileHandler;

struct sCount {
    unsigned int wordsEndingInConsoant;
    unsigned int wordsBeginningInVowel;
    unsigned int words;
};
typedef struct sCount Count;

struct sResults {
    char fileName[MAX_FILE_NAME_SIZE]; 
    Count count;
};
typedef struct sResults Results;


int sm_registerFiles(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE]);


bool sm_getChunkOfData(int id, unsigned char data[DATA_BUFFER_SIZE], unsigned int *size, FileHandler *fileHandler);


void sm_registerResult(int id, FileHandler fileHandler, Count *count);


void sm_getResults(Results *results);


int sm_close();

#endif /* SHARED_MEMORY_H */
