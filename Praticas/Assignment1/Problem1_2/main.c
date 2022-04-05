#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "sharedMemory.h"
#include "utf8.h"
#include "probConst.h"

/** \brief worker life cycle routine */
static void * work(void *args);

/** \brief print results */
void printResults(Results *results, unsigned int numberFiles);

/** \brief worker threads return status array */
int statusWorkers[N];

int main(int argc, char *argv[])
{
    //Parse args
    if (argc == 1)
    {
        fprintf(stderr, "USAGE: ./countWords fileName [fileName ...]\n");
        return 1;
    }
    //Save file names and count in shared memory
    int nFiles = argc-1;
    char fileNames[nFiles][MAX_FILE_NAME_SIZE];

    for(int i = 0; i < nFiles; i++)
    {
        strcpy(fileNames[i], argv[i+1]);
    }
    if(sm_registerFiles(nFiles, fileNames) == FAILURE)
    {
        fprintf(stderr, "Fail to initialize shared memory");
        exit(EXIT_FAILURE);
    }

    //Create workers
    pthread_t workers[N];
    unsigned int workersID[N];

    for (int i = 0; i < N; i++)
        workersID[i] = i;


    double startTime = 0, endTime = 0, elapsedTime = 0;
    startTime = ((double)clock()) / CLOCKS_PER_SEC;

    for (int i = 0; i < N; i++)
        if (pthread_create(&workers[i], NULL, work, (void *) &workersID[i]) != 0) /* thread producer */
        {
            perror("Error on creating workers");
            exit(EXIT_FAILURE);
        }

    //Wait for all workers to finish
    int *executionStatus;
    for (int i = 0; i < N; i++)
    {
        if (pthread_join(workers[i], (void *)&executionStatus) != 0) /* thread producer */
        {
            perror("error on waiting for thread producer");
            exit(EXIT_FAILURE);
        }
        printf("thread worker, with id %u, has terminated: ", i);
        printf("its status was %d\n", *executionStatus);
    }

    endTime = ((double)clock()) / CLOCKS_PER_SEC;
    elapsedTime += endTime - startTime;
    printf("\nElapsed time = %.6f s\n", elapsedTime);

    struct sResults results[nFiles];
    sm_getResults(results);
    printResults(results, nFiles);

    sm_close();
    exit(EXIT_SUCCESS);
}

void printResults(Results *results, unsigned int numberFiles)
{
    for(int i = 0; i < numberFiles; i++)
    {
        fprintf(stdout,
        "\nFile name: %s\n"
        "Total number of words = %d\n"
        "N. of words beginning with a vowel = %d\n"
        "N. of words ending with a consonant = %d\n",
        results[i].fileName, results[i].count.words, results[i].count.wordsBeginningInVowel, results[i].count.wordsEndingInConsoant);
    }
    
}

static void * work(void * args)
{
    while(true)
    {
        int id = *((int *) args);
        unsigned char data[DATA_BUFFER_SIZE];
        unsigned int size;
        FileHandler fileHandler;
        bool workToDo = sm_getChunkOfData(id, data, &size, &fileHandler);

        if(!workToDo) //end work life cycle if there is no more work to do
        {
            statusWorkers[id] = EXIT_SUCCESS;
            pthread_exit(&statusWorkers[id]);
        }

        unsigned int totalWords = 0;
        unsigned int totalWordsEndingInConsoant = 0;
        unsigned int totalWordsBeginningInVowel = 0;
        enum CharacterType charType;
        enum CharacterType lastCharType;
        bool inWord = false;


        unsigned int utf8Char;
        int utf8CharSize;
        int dataIdx = 0;
        // Block of text
        do
        {
            utf8Char = data[dataIdx];
            utf8CharSize = getUTF8CharSize(data[dataIdx]);
            
            for(int i = 1; i < utf8CharSize; i++)
            {
                utf8Char = (utf8Char << 8) | data[dataIdx + i];
            }
            dataIdx += utf8CharSize;
            charType = getUTF8CharType(utf8Char);

            switch (inWord)
            {
            case false:
                switch (charType)
                {
                case VOWEL:
                    totalWordsBeginningInVowel++;
                case CONSOANT:
                case UNDERSCORE:
                case DIGIT:
                    totalWords++;
                    inWord = true;
                    break;

                case ERROR:
                    fprintf(stderr, "ERROR reading character\n");
                    statusWorkers[id] = EXIT_FAILURE;
                    pthread_exit(&statusWorkers[id]);
                    break;

                default:
                    break;
                }

                break;

            case true:
                switch (charType)
                {
                case DELIMITER:
                    if (lastCharType == CONSOANT)
                        totalWordsEndingInConsoant++;

                    inWord = false;
                    break;

                case ERROR:
                    fprintf(stderr, "ERROR reading character\n");
                    statusWorkers[id] = EXIT_FAILURE;
                    pthread_exit(&statusWorkers[id]);
                    break;

                default:
                    break;
                }
                break;
            }
            /*if(charType == VOWEL)
                printf("VOWEL\n");
            if(charType == CONSOANT)
                printf("CONSOANT\n");
            if(charType == UNDERSCORE)
                printf("UNDERSCORE\n");
            if(charType == DIGIT)
                printf("DIGIT\n");
            if(charType == DELIMITER)
                printf("DELIMITER\n");
            if(charType == ERROR)
                printf("ERROR\n");         
            printf("%x\n", utf8Char); */
            if (charType != NOT_DEFINED && charType != ERROR) // ignore case NOT_DEFINED or ERROR
                lastCharType = charType; 

        } while (dataIdx < size);

        Count count;
        count.words = totalWords;
        count.wordsBeginningInVowel = totalWordsBeginningInVowel;
        count.wordsEndingInConsoant = totalWordsEndingInConsoant;
        
        sm_registerResult(id, fileHandler, &count);
    }
}