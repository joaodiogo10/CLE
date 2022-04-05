#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "sharedMemory.h"
#include "utf8.h"

static unsigned int fileIdx;

/** \brief total number of files */
static unsigned int numberOfFiles;

struct sFileHandler
{
    Count count;
    char *fileName;
    FILE *ptrFile;
};

static FileHandler handlers;

/** \brief flag to check if sharedMemory is initialized */
static bool initialized = false;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief worker threads return status array */
int statusWorkers[N];


int sm_registerFiles(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE])
{
    if (initialized)
    {
        fprintf(stderr, "Error shared memory already initialized");
        return FAILURE;
    }

    // verify file path size
    for (int i = 0; i < numberOfFiles; i++)
        if (strlen(files[i]) > MAX_FILE_NAME_SIZE)
        {
            fprintf(stderr, "Error initializing file path excess MAX_FILE_NAME_SIZE");
            return FAILURE;
        }

    numberOfFiles = nFiles;
    fileIdx = 0;

    handlers = (FileHandler) malloc(nFiles * sizeof(struct sFileHandler));
    if (handlers == NULL)
    {
        perror("malloc error");
        return FAILURE;
    }

    for (int i = 0; i < numberOfFiles; i++)
    {
        handlers[i].fileName = (char *) calloc(MAX_FILE_NAME_SIZE, sizeof(char));
        handlers[i].count.words = 0;
        handlers[i].count.wordsBeginningInVowel = 0;
        handlers[i].count.wordsEndingInConsoant = 0;

        strcpy(handlers[i].fileName , files[i]);

        //Open file stream
        FILE *ptrFile;
        ptrFile = fopen(handlers[i].fileName , "rb");

        if (ptrFile == NULL)
        {
            perror("fopen error");
            return FAILURE;
        }

        handlers[i].ptrFile = ptrFile;
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


    //TODO: free memory
    return SUCCESS;
}


bool sm_getChunkOfData(int id, unsigned char data[DATA_BUFFER_SIZE], unsigned int *size, FileHandler *fileHandler)
{
    bool moreWorkToDo = true;
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; save error in errno ???
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    if (fileIdx < numberOfFiles)
    {
        *fileHandler = &handlers[fileIdx];
        FILE * file = handlers[fileIdx].ptrFile;

        //Get data from file
        if (( *size = fread(data, sizeof(char), DATA_BUFFER_SIZE, file) ) < DATA_BUFFER_SIZE)
        {
            if (ferror(file) != 0)
            {
                perror("error on entering monitor");
                statusWorkers[id] = EXIT_FAILURE;
                pthread_exit(&statusWorkers[id]);
            }

            //no more bytes in file get new file
            fileIdx++;
        }

        bool foundDelimiter = false;
        unsigned int goBackN = 0;
        
        //look for last delimiter character in buffer
        while(!foundDelimiter)
        {
            //find beginning of the character
            unsigned int characterSize;
            goBackN++;
            while(( characterSize = getUTF8CharSize(data[*size - goBackN]) ) == 0)
                goBackN++;

            //check if delimiter
            unsigned int utf8Char = data[*size - goBackN];
            for(int i = 1; i < characterSize; i++)
                utf8Char = (utf8Char << 8) | data[*size - goBackN + i ];

            if(getUTF8CharType(utf8Char) == DELIMITER)
            {
                foundDelimiter = true;
                (*size) = *size - goBackN + characterSize;       //count with last delimiter character

                if(fseek(file, (int)(-goBackN + characterSize), SEEK_CUR) != 0)
                {
                    statusWorkers[id] = EXIT_FAILURE;
                    pthread_exit(&statusWorkers[id]);
                }
            }
        }
    }
    //no more files
    else
        moreWorkToDo = false;
    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; save error in errno ???
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    return moreWorkToDo;
} 


void sm_registerResult(int id, FileHandler fileHandler, Count *count)
{
    if (pthread_mutex_lock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; /* save error in errno ???*/
        perror("error on entering monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
    fileHandler->count.words += count->words;
    fileHandler->count.wordsBeginningInVowel += count->wordsBeginningInVowel;
    fileHandler->count.wordsEndingInConsoant += count->wordsEndingInConsoant;
    if (pthread_mutex_unlock(&accessCR) != 0)
    {
        // errno = statusWorkers[id]; /* save error in errno ???*/
        perror("error on exiting monitor");
        statusWorkers[id] = EXIT_FAILURE;
        pthread_exit(&statusWorkers[id]);
    }
}


void sm_getResults(Results *results)
{
    for(int i = 0; i < numberOfFiles; i++)
    {
        strcpy(results[i].fileName, handlers[i].fileName);
        results[i].count.words = handlers[i].count.words;
        results[i].count.wordsBeginningInVowel = handlers[i].count.wordsBeginningInVowel;
        results[i].count.wordsEndingInConsoant = handlers[i].count.wordsEndingInConsoant;
    }
}
