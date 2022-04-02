#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "probConst.h"
#include "sharedMemory.h"
#include "asciiConst.h"

enum CharacterType
{
    CONSOANT,
    UNDERSCORE,
    VOWEL,
    DIGIT,
    APOSTROPHE,
    DELIMITER,
    EOFILE,
    NOT_DEFINED,
    ERROR
};

/** \brief read UTF8 value and returns CharacterType */
enum CharacterType readUTF8Char(FILE *ptrFile, unsigned int *utf8Char);

void printResults(char *filePath, unsigned int totalWords, unsigned int totalWordsBeginningInVowel, unsigned int totalWordsEndingInConsoant);

/** \brief worker life cycle routine */
static void * work(void *args);

/** \brief worker threads return status array */
int statusWorkers[N];

int main(int argc, char *argv[])
{
    //Parse args
    if (argc == 1)
    {
        fprintf(stderr, "USAGE: ./countWords filePath [filePath ...]\n");
        return 1;
    }
    
    //Save file names and count in shared memory
    int nFiles = argc-1;
    char fileNames[nFiles][MAX_FILE_PATH_SIZE];
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

    unsigned int totalWordsEndingInConsoant;
    unsigned int totalWordsBeginningInVowel;
    unsigned int totalWords;

    sm_getResults(&totalWordsEndingInConsoant, &totalWordsBeginningInVowel, &totalWords);
    printResults(fileNames[0], totalWords, totalWordsBeginningInVowel, totalWordsEndingInConsoant);

    sm_close();
    exit(EXIT_SUCCESS);
}

void printResults(char *filePath, unsigned int totalWords, unsigned int totalWordsBeginningInVowel, unsigned int totalWordsEndingInConsoant)
{
    fprintf(stdout,
            "\nFile name: %s\n"
            "Total number of words = %d\n"
            "N. of words beginning with a vowel = %d\n"
            "N. of words ending with a consonant = %d\n",
            filePath, totalWords, totalWordsBeginningInVowel, totalWordsEndingInConsoant);
}

static void * work(void * args)
{
    while(true)
    {
        char filePath[MAX_FILE_PATH_SIZE];
        int id = *((int *) args);
        bool workToDo = sm_getFileToProcess(id, filePath);
        //end work life cycle if there is no more work to do
        if(!workToDo)
        {
            statusWorkers[id] = EXIT_SUCCESS;
            pthread_exit(&statusWorkers[id]);
        }
        
        printf("File name: %s, thread: %d\n", filePath, id);
        
        FILE *ptrFile;
        ptrFile = fopen(filePath, "rb");

        if (ptrFile == NULL)
        {
            fprintf(stderr, "Error opening file \"%s\"!\n", filePath);
            statusWorkers[id] = EXIT_SUCCESS;
            pthread_exit(&statusWorkers[id]);
        }

        unsigned int totalWords = 0;
        unsigned int totalWordsEndingInConsoant = 0;
        unsigned int totalWordsBeginningInVowel = 0;
        unsigned int utf8Char;
        enum CharacterType charType;
        enum CharacterType lastCharType;
        bool inWord = false;

        // Block of text
        do
        {
            charType = readUTF8Char(ptrFile, &utf8Char);
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

            if (charType != NOT_DEFINED && charType != ERROR) // ignore case NOT_DEFINED or ERROR
                lastCharType = charType;

        } while (charType != EOFILE);

        sm_addTotalWords(id, totalWords);
        sm_addTotalWordsBeginningInVowel(id, totalWordsBeginningInVowel);
        sm_addTotalWordsEndingInConsoant(id, totalWordsEndingInConsoant);
        fclose(ptrFile);
    }
}

enum CharacterType readUTF8Char(FILE *ptrFile, unsigned int *utf8Char)
{
    unsigned char buffer[3];
    if (fread(&buffer, sizeof(char), 1, ptrFile) != sizeof(char))
    {
        if (ferror(ptrFile) != 0)
            return ERROR;

        if (feof(ptrFile) != 0)
            return EOFILE;
    }

    unsigned char tmp = buffer[0] & 0xF8;
    *utf8Char = buffer[0];

    if (tmp >> 7 == 0) // 1 byte character
    {
        // do nothing
    }
    else if (tmp >> 5 == 0b110) // 2 bytes character
    {
        if (fread(&buffer, sizeof(char), 1, ptrFile) != sizeof(char))
            return ERROR;

        *utf8Char = (*utf8Char << 8) | buffer[0];
    }
    else if (tmp >> 4 == 0b1110) // 3 bytes character
    {
        if (fread(&buffer, sizeof(char), 2, ptrFile) != sizeof(char) * 2)
            return ERROR;

        *utf8Char = (*utf8Char << 16) | (buffer[0] << 8) | buffer[1];
    }
    else if (tmp >> 3 == 0b1110) // 4 bytes character
    {
        if (fread(&buffer, sizeof(char), 3, ptrFile) != sizeof(char) * 3)
            return ERROR;

        *utf8Char = (*utf8Char << 24) | (buffer[0] << 16) | (buffer[1] << 8) | buffer[0];
    }
    // invalid
    else
    {
        return ERROR;
    }

    // Check if is a delimiter
    for (int i = 0; i < sizeof(delimiters) / sizeof(*delimiters); i++)
    {
        if (delimiters[i] == *utf8Char)
            return DELIMITER;
    }

    // check if is a apostrophe
    if (*utf8Char == apostrophe)
        return APOSTROPHE;

    // Check if is a vowel
    for (int i = 0; i < sizeof(vowels) / sizeof(*vowels); i++)
    {
        if (vowels[i] == *utf8Char)
            return VOWEL;
    }

    // Check if is a consoant
    for (int i = 0; i < sizeof(specialConsoants) / sizeof(*specialConsoants); i++)
    {
        if (specialConsoants[i] == *utf8Char)
        {
            return CONSOANT;
        }
    }
    if ((ASCII_LAST_LOWER_CASE_LETTER >= *utf8Char && *utf8Char >= ASCII_FIRST_LOWER_CASE_LETTER) ||
        (ASCII_LAST_UPPER_CASE_LETTER >= *utf8Char && *utf8Char >= ASCII_FIRST_UPPER_CASE_LETTER))
    {
        return CONSOANT;
    }

    // Check if is a digit
    if (*utf8Char >= 0x30 && *utf8Char <= 0x39)
        return DIGIT;

    // Check if is an underscore
    if (*utf8Char == underscore)
    {
        return UNDERSCORE;
    }

    return NOT_DEFINED;
}