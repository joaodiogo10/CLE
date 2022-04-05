#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H
#include "probConst.h"

#define SUCCESS 1
#define FAILURE 0

struct sFileHandler;
typedef struct sFileHandler* FileHandler;

int sm_initialize(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE]);

bool sm_getFileToProcess(int id, FileHandler *fileHandler);

int sm_getFileName(FileHandler fileHandler, char fileName[MAX_FILE_NAME_SIZE]);

void sm_addTotalWordsEndingInConsoant(int id, unsigned int numberWords, FileHandler fileHandler);

void sm_addTotalWordsBeginningInVowel(int id, unsigned int numberWords, FileHandler fileHandler);

void sm_addTotalWords(int id, unsigned int numberWords, FileHandler fileHandler);

void sm_getResults(unsigned int *wordsEndingInConsoant, unsigned int *wordsBeginningInVowel,
                   unsigned int *words);

int sm_close();

#endif /* SHARED_MEMORY_H */
