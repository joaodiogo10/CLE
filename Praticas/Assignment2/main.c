#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "probConst.h"
#include "textFiles.h"
#include "utf8.h"

/** \brief Data chunk process by a worker a respective file handler */
struct sChunk
{
    unsigned char data[DATA_BUFFER_SIZE];
    FileHandler handler;
    Result result; /*!< Number of words ending in consoant */
                   /*!< Number of words beginning in vowel */
                   /*!< Total number of words */
};
typedef struct sChunk Chunk;

int main(int argc, char *argv[])
{
    int rank, nProc, nWorkers;
    int nFiles;
    char fileNames[nFiles][MAX_FILE_NAME_SIZE];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    nWorkers = nProc - 1;

    if (nProc <= 1)
    {
        if (rank == 0)
            printf("Wrong number of processes! It must be greater than 1.\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    //------------------------
    //-------Dispatcher-------
    //------------------------
    if (rank == 0)
    {
        // parseFiles
        if (argc == 1)
        {
            fprintf(stderr, "USAGE: ./countWords fileName [fileName ...]\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        nFiles = argc - 1;

        for (int i = 0; i < nFiles; i++)
        {
            if (strlen(fileNames[nFiles]) > MAX_FILE_NAME_SIZE)
            {
                printf("File path is too long!\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
            strcpy(fileNames[i], argv[i + 1]);
        }

        int ready[nWorkers];
        Chunk dataChunks[nWorkers];
        MPI_Request requests[nWorkers];

        for (int i = 0; i < nWorkers; i++)
            ready[nWorkers] = true;

        bool first = true;
        bool moreChunks = true;

        while (moreChunks)
        {
            for (int i = 0; i < nWorkers; i++)
            {
                if (ready[i])
                {
                    if (!first)
                        tf_registerResult(dataChunks[i].handler, dataChunks[i].result);

                    moreChunks = readChunk(&dataChunks);
                    if (!moreChunks)
                        break;

                    MPI_Isend((void *)dataChunks[i].data, DATA_BUFFER_SIZE, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD, NULL);
                    MPI_Irecv((void *)dataChunks[i].result, 3, MPI_UINT32_T, i + 1, 0, MPI_COMM_WORLD, &requests[i]);

                    ready[i] = false;
                }

                MPI_Test(&requests[i], &ready[i], MPI_STATUS_IGNORE);
            }
            first = false;
        }

        // Waiting pending workers (blocking)
        for (int i = 0; i < nWorkers; i++)
        {
            while (!ready[i])
                MPI_Test(&requests[i], &ready[i], MPI_STATUS_IGNORE);

            tf_registerResult(dataChunks[i].handler, dataChunks[i].result);
        }

        // Send termination condition, i.e, dataChunk size 0 (blocking)
        unsigned char finish[DATA_BUFFER_SIZE];
        finish[DATA_BUFFER_SIZE - 1] = 0;
        finish[DATA_BUFFER_SIZE] = 0;

        for (int i = 0; i < nWorkers; i++)
        {
            MPI_Isend((void *)finish, DATA_BUFFER_SIZE, MPI_CHAR, i + 1, 0, MPI_COMM_WORLD, NULL);
        }

        // Print results

        Result results;
        int numberOfFiles = tf_getResults(results);
        for (int i = 0; i < numberOfFiles; i++)
        {
            fprintf(stdout,
                    "\nFile name: %s\n"
                    "Total number of words = %d\n"
                    "N. of words beginning with a vowel = %d\n"
                    "N. of words ending with a consonant = %d\n",
                    fileNames[i], results[i][0], results[i][1], results[i][2]);
        }
    }

    //------------------------
    //---------Worker---------
    //------------------------
    else
    {
        Result result;
        unsigned char data[DATA_BUFFER_SIZE];

        while (true)
        {
            MPI_Recv( (void *) data, DATA_BUFFER_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            uint16_t dataSize = (((uint16_t)data[DATA_BUFFER_SIZE - 1]) << 8) & ((uint16_t)data[DATA_BUFFER_SIZE - 1]);

            // Check is there is more work to do
            if (dataSize == 0)
                break;

            processChunkOfData(data, dataSize, result);

            MPI_Send( (void *) result, 3, MPI_UINT32_T, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

void processChunkOfData(unsigned char data[DATA_BUFFER_SIZE], uint16_t dataSize, Result result)
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
    result[0] = totalWords;
}