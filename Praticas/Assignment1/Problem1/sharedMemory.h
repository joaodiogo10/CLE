#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H
#include "probConst.h"

#define SUCCESS 1
#define FAILURE 0

int sm_initialize(int nFiles, char files[nFiles][MAX_FILE_PATH_SIZE]);

bool sm_getFileToProcess(int id, char *fileName);

void sm_addTotalWordsEndingInConsoant(int id, unsigned int numberWords);

void sm_addTotalWordsBeginningInVowel(int id, unsigned int numberWords);

void sm_addTotalWords(int id, unsigned int numberWords);

void sm_getResults(unsigned int *wordsEndingInConsoant, unsigned int *wordsBeginningInVowel,
                   unsigned int *words);

int sm_close();

#endif /* SHARED_MEMORY_H */
