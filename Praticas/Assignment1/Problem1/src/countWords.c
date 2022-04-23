#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "sharedMemory.h"
#include "utf8.h"
#include "probConst.h"

/**
 *  \file countWords.c
 *
 *  \brief Text processing in Portuguese program  
 *
 *  This program reads in succession several text files text#.txt whose names are provided in
 *  the command line and prints a listing of total number of words, number of words beginning with a
 *  vowel and number of words ending with a consonant for each of the supplied files.
 *  
 *  To carry out this task 1 or more concurrent worker threads are launched.  
 * 
 *  \author João Diogo Ferreira, João Tiago Rainho - April 2022
 */


/** \brief Worker life cycle routine. */
static void * work(void *args);


/** \brief Print results. */
void printResults(const Results results);


/** \brief worker threads return status array */
int statusWorkers[N];

/** \brief Main thread.
 *  
 *  The role of main thread is to get the data file names by processing the command line and storing them
 *  in the shared region, creating the worker threads and waiting for their termination, and printing the results
 *  o the processing.
*/
int main(int argc, char *argv[])
{
    //Parse file names
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
    if(sm_initialize(nFiles, fileNames) == FAILURE)
    {
        fprintf(stderr, "Fail to initialize shared memory");
        exit(EXIT_FAILURE);
    }

    //Create workers
    pthread_t workers[N];
    unsigned int workersID[N];

    for (int i = 0; i < N; i++)
        workersID[i] = i;

    //Determine executing start time
    struct timespec startTime, endTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

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

    //Determine executing time
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    printf ("\nElapsed time = %.6f s\n",  (endTime.tv_sec - startTime.tv_sec) / 1.0 + (endTime.tv_nsec - startTime.tv_nsec) / 1000000000.0);

    //Get results
    struct sResults results[nFiles];
    sm_getResults(results);

    //print results for each file
    for(int i = 0; i < nFiles; i++)
        printResults(results[i]);

    sm_close();
    exit(EXIT_SUCCESS);
}

/** \brief Prints results of a given file.
 * 
 *  This function prints to stdout the corresponding processing results of 
 *  a given file.
 * 
 *  \param results Structure contain file results
*/
void printResults(const Results results)
{
    fprintf(stdout,
    "\nFile name: %s\n"
    "Total number of words = %d\n"
    "N. of words beginning with a vowel = %d\n"
    "N. of words ending with a consonant = %d\n",
    results.fileName, results.count.words, results.count.wordsBeginningInVowel, results.count.wordsEndingInConsoant);
}

/** \brief Worker routine.
 * 
 *  The role of worker thread is to carry out the processing itself:
 *  First it requests a piece of data to process, processes it and delivers the
 *  results. 
 *  
 *  At the end the processing, the total number of words, words beginning in vowel
 *  and words ending in consonant, of the corresponding piece of data, is determined.
 * 
 *  \param args Pointer to defined worker identification (int)
*/
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
            if (charType != NOT_DEFINED) // ignore case NOT_DEFINED
                lastCharType = charType; 

        } while (dataIdx < size);

        Count count;
        count.words = totalWords;
        count.wordsBeginningInVowel = totalWordsBeginningInVowel;
        count.wordsEndingInConsoant = totalWordsEndingInConsoant;
        
        sm_registerResult(id, fileHandler, &count);
    }
}