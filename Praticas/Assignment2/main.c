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

void processChunkOfData(uint8_t data[DATA_BUFFER_SIZE], uint16_t dataSize, Result result);

void *codeReadingThread(void *args);

void *codeProxyThread(void *args);

int main(int argc, char *argv[])
{
    // validate input arguments
    if (argc == 1)
    {
        fprintf(stderr, "USAGE: ./countWords fileName [fileName ...]\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int rank, nProc, nWorkers;
    int nFiles = argc - 1;
    char fileNames[nFiles][MAX_FILE_NAME_SIZE];
    int provided;
    
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE)
        fprintf(stderr, "Warning MPI did not provide MPI_THREAD_FUNNELED\n");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    

    if (nProc <= 1)
    {
        if (rank == 0)
            printf("Wrong number of processes! It must be greater than 1.\n");
        MPI_Finalize();
        return EXIT_FAILURE;
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
                printf("File path is too long!\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
            strcpy(fileNames[i], argv[i + 1]);
        }

        int status = tf_initialize(nFiles, fileNames);
        if (status == FAILURE)
        {
            printf("File to initialize text files!\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        // Determine executing start time
        struct timespec startTime, endTime;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        // create reading thread
        pthread_t readingThread;
        pthread_create(&readingThread, NULL, codeReadingThread, NULL);

        // launch proxy threads
        pthread_t proxyThread[nWorkers];
        int workerIDs[nWorkers];

        for (int i = 0; i < nWorkers; i++)
        {
            workerIDs[i] = i + 1;
            pthread_create(&proxyThread[i], NULL, codeProxyThread, (void *)&workerIDs[i]);
        }

        // wait for reading threads
        pthread_join(readingThread, NULL);

        // join working threads
        for (int i = 0; i < nWorkers; i++)
        {
            pthread_join(proxyThread[i], NULL);
        }

        // Determine executing time
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        printf("\nElapsed time = %.6f s\n", (endTime.tv_sec - startTime.tv_sec) / 1.0 + (endTime.tv_nsec - startTime.tv_nsec) / 1000000000.0);

        // Print results
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
            if (dataSize == 0xFFFF)
                break;

            processChunkOfData(data, dataSize, result);
            MPI_Send((void *)result, 3, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
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
        int moreChunk = getChunk(&dataChunk);

        if (!moreChunk)
        {
            // Send termination condition, i.e., dataChunk size 0xFFFF
            uint8_t finish[DATA_BUFFER_SIZE];
            finish[DATA_BUFFER_SIZE - 2] = 0xFF;
            finish[DATA_BUFFER_SIZE - 1] = 0xFF;

            MPI_Send((void *)finish, DATA_BUFFER_SIZE, MPI_UINT8_T, workerId, 0, MPI_COMM_WORLD);
            break;
        }

        // send data chunk
        MPI_Send((void *)&dataChunk->data, DATA_BUFFER_SIZE, MPI_UINT8_T, workerId, 0, MPI_COMM_WORLD);

        // receive result
        MPI_Recv((void *)dataChunk->result, 3, MPI_UINT32_T, workerId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        tf_registerResult(dataChunk->handler, dataChunk->result);
        free(dataChunk);
    }
    return NULL;
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
            printf("Error reading data chunk!\n");
            MPI_Finalize();
            return NULL;
        }

        if (moreChunks)
            putChunk(dataChunk);
    }

    doneReading();

    return NULL;
}