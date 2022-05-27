#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "probConst.h"
#include "textFiles.h"
#include "utf8.h"
#include "fifo.h"

/**
 *  \file main.c
 *
 *  \brief Text processing in Portuguese program  
 *
 *  This program reads in succession several text files text#.txt whose names are provided in
 *  the command line and prints a listing of total number of words, number of words beginning with a
 *  vowel and number of words ending with a consonant for each of the supplied files.
 *  
 *  \author João Diogo Ferreira, João Tiago Rainho - May 2022
 */

/** \brief Reading thread return status */
int statusReadingThread;

/** \brief Proxy threads return status */
int * statusProxyThread;

/** \brief Gives the couting results of a given chunk of data */
void processChunkOfData(uint8_t data[DATA_BUFFER_SIZE], uint16_t dataSize, Result result);

/** \brief Execution code of file reader thread*/
void *codeReadingThread(void *args);

/** \brief Execution code of proxy thread */
void *codeProxyThread(void *args);

/** \brief Send termination condition message to the target worker */
void sendTerminationCondition(unsigned int workerId);

int main(int argc, char *argv[])
{
    // validate input arguments
    if (argc == 1)
    {
        fprintf(stderr, "USAGE: ./countWords fileName [fileName ...]\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    int rank, nProc, nWorkers;
    int nFiles = argc - 1;
    char fileNames[nFiles][MAX_FILE_NAME_SIZE];
    int provided;
    
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE)
    {
        fprintf(stderr, "Warning MPI did not provide MPI_THREAD_MULTIPLE\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    
    if (nProc <= 1)
    {
        if (rank == 0)
            printf("Wrong number of processes! It must be greater than 1.\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    nWorkers = nProc - 1;
    //------------------------
    //-------Dispatcher-------
    //------------------------
    if (rank == 0)
    {
        // parseFiles
        for (int i = 0; i < nFiles; i++)
        {
            if (strlen(argv[i + 1]) > MAX_FILE_NAME_SIZE)
            {
                fprintf(stderr, "File path is too long!\n");
                for(int n = 1; n <= nWorkers; n++)
                    sendTerminationCondition(n);
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            strcpy(fileNames[i], argv[i + 1]);
        }

        int status = tf_initialize(nFiles, fileNames);
        if (status == FAILURE)
        {
            fprintf(stderr, "Failed to initialize text files!\n");
            for(int n = 1; n <= nWorkers; n++)
                sendTerminationCondition(n);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
        
        //intialize proxies return status storage
        statusProxyThread = (int *) malloc(nWorkers * sizeof(int));
        if(statusProxyThread == NULL)
        {
            fprintf(stderr  , "Failed to alocate proxies return status storage\n");
            for(int n = 1; n <= nWorkers; n++)
                sendTerminationCondition(n);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        } 

        // Determine executing start time
        struct timespec startTime, endTime;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        // launch reading thread
        pthread_t readingThread;
        if (pthread_create(&readingThread, NULL, codeReadingThread, NULL) != 0)
        {
            fprintf(stdout, "Error on creating reader thread\n");
            for(int n = 1; n <= nWorkers; n++)
                sendTerminationCondition(n);
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }

        // launch proxy threads
        pthread_t proxyThread[nWorkers];
        int workerIDs[nWorkers];

        for (int i = 0; i < nWorkers; i++)
        {
            workerIDs[i] = i + 1;
            if (pthread_create(&proxyThread[i], NULL, codeProxyThread, (void *)&workerIDs[i]) != 0)
            {
                fprintf(stdout, "Error on creating proxy thread\n");
                for(int n = i + 1; n <= nWorkers; n++)
                    sendTerminationCondition(n);

                doneReading();
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
        }

        // wait for reading threads
        int *executionStatus;
        bool success = true;
        if(pthread_join(readingThread, (void *) &executionStatus) != 0)
        {
            fprintf(stdout, "Error on waiting reader thread\n");
            MPI_Finalize();
            exit(EXIT_FAILURE);
        }
        printf("thread reader, has terminated: ");
        printf("its status was %d\n", *executionStatus);
        if(*executionStatus != 0)
            success = false;

        // join working threads
        for (int i = 0; i < nWorkers; i++)
        {
            if(pthread_join(proxyThread[i], (void *) &executionStatus) != 0)
            {
                fprintf(stdout, "Error on waiting worker thread %d\n", i);
                MPI_Finalize();
                exit(EXIT_FAILURE);
            }
            printf("thread proxy, with id %u, has terminated: ", i);
            printf("its status was %d\n", *executionStatus);
            if(*executionStatus != 0)
                success = false;
        }

        // Determine executing time
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        printf("\nElapsed time = %.6f s\n", (endTime.tv_sec - startTime.tv_sec) / 1.0 + (endTime.tv_nsec - startTime.tv_nsec) / 1000000000.0);


        // Print results
        if(success)
        {
            Result results[nFiles];
            tf_getResults(results);
            for (int i = 0; i < nFiles; i++)
            {
                fprintf(stdout,
                        "\nFile name: %s\n"
                        "Total number of words = %d\n"
                        "N. of words beginning with a vowel = %d\n"
                        "N. of words ending with a consonant = %d\n",
                        fileNames[i], results[i][2], results[i][1], results[i][0]);
            }
        }
        else
        {
            fprintf(stderr, "Unable to get results something went wrong during the execution of the threads\n");
        }
        
        tf_close();
    }

    //------------------------
    //---------Worker---------
    //------------------------
    else
    {
        Result result;
        uint8_t data[DATA_BUFFER_SIZE];

        while (true)
        {
            MPI_Recv((void *)data, DATA_BUFFER_SIZE, MPI_UINT8_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            uint16_t dataSize = (((uint16_t)data[DATA_BUFFER_SIZE - 1]) << 8) | ((uint16_t)data[DATA_BUFFER_SIZE - 2]);
            // Check is there is more work to do
            if (dataSize == 0x0000)
                break;

            processChunkOfData(data, dataSize, result);
            MPI_Send((void *)result, 3, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    exit(EXIT_FAILURE);
}

void processChunkOfData(uint8_t data[DATA_BUFFER_SIZE], uint16_t dataSize, Result result)
{
    // process Chunk of data
    unsigned int totalWordsEndingInConsoant = 0;
    unsigned int totalWordsBeginningInVowel = 0;
    unsigned int totalWords = 0;

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

        for (int i = 1; i < utf8CharSize; i++)
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
        if (charType != NOT_DEFINED) // ignore case NOT_DEFINED
            lastCharType = charType;

    } while (dataIdx < dataSize);

    result[0] = totalWordsEndingInConsoant;
    result[1] = totalWordsBeginningInVowel;
    result[2] = totalWords;
}

void *codeProxyThread(void *args)
{
    Chunk * dataChunk;
    unsigned int workerId = *((int *)args);

    while (true)
    {
        int moreChunk = getChunk(statusProxyThread[workerId], &dataChunk);

        if (!moreChunk)
        {
            sendTerminationCondition(workerId);
            break;
        }

        // send data chunk
        MPI_Send((void *)&dataChunk->data, DATA_BUFFER_SIZE, MPI_UINT8_T, workerId, 0, MPI_COMM_WORLD);

        // receive result
        MPI_Recv((void *)dataChunk->result, 3, MPI_UINT32_T, workerId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        tf_registerResult(dataChunk->handler, dataChunk->result);
        free(dataChunk);
    }
    statusProxyThread[workerId] = EXIT_SUCCESS;
    pthread_exit(&statusProxyThread[workerId]);
}

void *codeReadingThread(void *args)
{

    bool moreChunks = true;
    while (moreChunks)
    {
        Chunk *dataChunk = (Chunk *)malloc(sizeof(Chunk));
        int status = tf_readChunk(dataChunk->data, &(dataChunk->handler), &moreChunks);
        if (status == FAILURE)
        {
            fprintf(stderr, "Error reading data chunk!\n");
            doneReading();
            statusReadingThread = EXIT_FAILURE;
            pthread_exit(&statusReadingThread);
        }

        if (moreChunks)
            putChunk(dataChunk);
    }

    doneReading();
    statusReadingThread = EXIT_SUCCESS;
    pthread_exit(&statusReadingThread);
}

void sendTerminationCondition(unsigned int workerId)
{
    // Send termination condition, i.e., dataChunk size 0x0000
    uint8_t finish[DATA_BUFFER_SIZE];
    finish[DATA_BUFFER_SIZE - 2] = 0x00;
    finish[DATA_BUFFER_SIZE - 1] = 0x00;

    MPI_Send((void *)finish, DATA_BUFFER_SIZE, MPI_UINT8_T, workerId, 0, MPI_COMM_WORLD);
}
