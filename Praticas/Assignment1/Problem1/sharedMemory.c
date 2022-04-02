#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "sharedMemory.h"

static unsigned int fileIdx;


/** \brief total number of files */
static unsigned int numberOfFiles;

/** \brief name of the files to process */
static char **fileNames;

static unsigned int totalWordsEndingInConsoant;

static unsigned int totalWordsBeginningInVowel;

static unsigned int totalWords;

struct sFileHandler
{
    unsigned int totalWordsEndingInConsoant;
    unsigned int totalWordsBeginningInVowel;
    unsigned int totalWords;
    char *fileName;
};
typedef struct sFileHandler FileHandler;

static FileHandler *handlers;

/** \brief flag to check if sharedMemory is initialized */
static bool initialized = false;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief worker threads return status array */
int statusWorkers[N];

int sm_initialize( int nFiles, char files[nFiles][MAX_FILE_PATH_SIZE])
{
    if (initialized)
    {
        fprintf(stderr, "Error shared memory already initialized");
        return FAILURE;
    }

    // verify file path size
    for (int i = 0; i < numberOfFiles; i++)
        if (strlen(fileNames[i]) > MAX_FILE_PATH_SIZE)
        {
            fprintf(stderr, "Error initializing file path excess MAX_FILE_PATH_SIZE");
            return FAILURE;
        }

    totalWordsEndingInConsoant = totalWordsBeginningInVowel = totalWords = 0;
    numberOfFiles = nFiles;
    fileIdx = 0;

    handlers = (FileHandler *) malloc(nFiles * sizeof(FileHandler));
    if (handlers == NULL)
    {
        perror("Malloc error");
        return FAILURE;
    }
    
    fileNames =  (char **) calloc(nFiles, sizeof(char *));
    if (fileNames == NULL)
    {
        perror("Malloc error");
        return FAILURE;
    }

    for (int i = 0; i < numberOfFiles; i++)
    {
        fileNames[i] = (char *) calloc(MAX_FILE_PATH_SIZE, sizeof(char));
        if (fileNames[i] == NULL) // dealocate memory?
        {
            perror("Malloc error");
            return FAILURE;
        }

        handlers[i].fileName = (char *) calloc(MAX_FILE_PATH_SIZE, sizeof(char));
        
        handlers[i].totalWords = 0;
        handlers[i].totalWordsBeginningInVowel = 0;
        handlers[i].totalWordsEndingInConsoant = 0;

        strcpy(fileNames[i], files[i]);
        strcpy(handlers[i].fileName , files[i]);
    }
    initialized = true;

    return SUCCESS;
}

int sm_close()
{
    if (!initialized)
    {
        fprintf(stderr, "Error shared memory not initialized");
        return FAILURE;
    }

    for (int i = 0; i < numberOfFiles; i++)
    {
        free((void *) fileNames[i]);
    }

    free(fileNames);

    return SUCCESS;
}

bool sm_getFileToProcess(int id, char *fileName)
{
    bool moreWork = false;
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; /* save error in errno ???*/
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    if (fileIdx < numberOfFiles)
    {
        strcpy(fileName, fileNames[fileIdx]);
        fileIdx++;
        moreWork = true;
    }
    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; /* save error in errno ???*/
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }

    return moreWork;
}

void sm_addTotalWordsEndingInConsoant(int id, unsigned int numberWords)
{
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    totalWordsEndingInConsoant += numberWords;

    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
}

void sm_addTotalWordsBeginningInVowel(int id, unsigned int numberWords)
{
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    totalWordsBeginningInVowel += numberWords;
    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
}

void sm_addTotalWords(int id, unsigned int numberWords)
{
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    totalWords += numberWords;
    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
}

void sm_getResults(unsigned int *wordsEndingInConsoant, unsigned int *wordsBeginningInVowel,
                   unsigned int *words)
{
    *wordsEndingInConsoant = totalWordsEndingInConsoant;
    *wordsBeginningInVowel = totalWordsBeginningInVowel;
    *words = totalWords;
}