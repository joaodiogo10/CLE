#include <stdio.h>
#include <stdlib.h>
#include "textFiles.h"
#include "utf8.h"

/** \brief Indicates current file being processed */
static unsigned int fileIdx;

/** \brief total number of files */
static unsigned int numberOfFiles;

/** \brief Information regarding a file and it's counting results */
struct sFileHandler
{
    Result count;       /*!< Counting results */
    char *fileName;     /*!< File name */
    FILE *ptrFile;      /*!< File stream */
};

/** \brief List of file handlers */
static FileHandler handlers;

/** \brief flag to check if text files is initialized */
static bool initialized = false;

int tf_initialize(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE])
{
    if (initialized)
    {
        fprintf(stderr, "Error text files already initialized");
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
        handlers[i].count[0] = 0; //Number of words ending in consoant
        handlers[i].count[1] = 0; //Number of words beginning in vowel
        handlers[i].count[2] = 0; //Total number of words

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

int tf_close()
{
    for(int i = 0; i < numberOfFiles; i++)
    {
        free(handlers[i].fileName);
        fclose(handlers[i].ptrFile);
    }
    free(handlers);

    return SUCCESS;
}

int tf_readChunk(uint8_t data[DATA_BUFFER_SIZE], FileHandler *fileHandler, bool *moreWork)
{
    bool moreWorkToDo = true;
    uint16_t size = 0;

    if (fileIdx < numberOfFiles)
    {
   
        *fileHandler = &handlers[fileIdx];
        FILE *file = handlers[fileIdx].ptrFile;
        
        //Get data from file
        if (( size = fread(data, sizeof(char), DATA_BUFFER_SIZE - 2, file) ) < DATA_BUFFER_SIZE - 2)
        {
            if (ferror(file) != 0)
            {
                fprintf(stderr, "Error on reading file");
                return FAILURE;
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
            while(( characterSize = getUTF8CharSize(data[size - goBackN]) ) == 0)
                goBackN++;

            //check if delimiter
            unsigned int utf8Char = data[size - goBackN];
            for(int i = 1; i < characterSize; i++)
                utf8Char = (utf8Char << 8) | data[size - goBackN + i ];

            if(getUTF8CharType(utf8Char) == DELIMITER)
            {
                foundDelimiter = true;
                (size) = size - goBackN + characterSize;       //count with last delimiter character

                if(fseek(file, (int)(-goBackN + characterSize), SEEK_CUR) != 0)
                {
                    fprintf(stderr, "Error on reading file");
                    return FAILURE;
                }
            }
        }
    }
    else
        moreWorkToDo = false;

    //store size in dataBuffer
    data[DATA_BUFFER_SIZE - 2] = (uint8_t) (size & 0x00FF);
    data[DATA_BUFFER_SIZE - 1] = (uint8_t) ((size >> 8) & 0x00FF);

    *moreWork = moreWorkToDo;
    return SUCCESS;
}

void tf_registerResult(FileHandler fileHandler, Result count)
{
    fileHandler->count[0] += count[0];  //Number of words ending in consoant
    fileHandler->count[1] += count[1];  //Number of words beginning in vowel
    fileHandler->count[2] += count[2];  //Total number of words
}

int tf_getResults(Result *results)
{
    for(int i = 0; i < numberOfFiles; i++)
    {
        results[i][0] = handlers[i].count[0]; //Number of words ending in consoant
        results[i][1] = handlers[i].count[1]; //Number of words beginning in vowel
        results[i][2] = handlers[i].count[2]; //Total number of words
    }

    return numberOfFiles;
}
