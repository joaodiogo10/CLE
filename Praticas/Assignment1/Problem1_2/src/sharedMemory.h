#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "probConst.h"

/**
 *  \file sharedMemory.h
 *
 *  \brief Shared memory header
 *  
 *  Synchronization based on monitors.
 * 
 *  Definition of the operations carried out by the workers:
 *     \li sm_getChunkOfData
 *     \li sm_registerResult.
 * 
 *  Definition of the operations carried out by the main thread:
 *     \li sm_initialize
 *     \li sm_close.
 *     \li sm_getResults
 * 
 *  \author João Diogo Ferreira, João Tiago Rainho - April 2022
 */

/** \brief Operation successful return code */
#define SUCCESS 1

/** \brief Operation failure return code */
#define FAILURE 0

/** \brief Opaque FileHandler used by workers to identify the target file */
typedef struct sFileHandler *FileHandler;

/** \brief List of counting results */
struct sCount
{
    unsigned int wordsEndingInConsoant;     /*!< Number of words ending in consoant */
    unsigned int wordsBeginningInVowel;     /*!< Number of words beginning in vowel */
    unsigned int words;                     /*!< Total number of words */
};  
typedef struct sCount Count;

/** \brief List of final counting results of a given file */
struct sResults
{
    char fileName[MAX_FILE_NAME_SIZE];      /*!< File name */
    Count count;                            /*!< Counting results */
};
typedef struct sResults Results;

/** \brief Shared memory initialization.
 *  
 *  Operation carried out by main thread.
 *  This function initializes shared memory resources and it must be called before calling any other
 *  shared memory function.
 *
 *  \param nFiles Total number of files
 *  \param files Array containing the names of all files
 *
 *  \returns FAILURE If an error occurs, otherwise SUCCESS
 *  \sa FAILURE
 *  \sa SUCCESS
 *  \sa MAX_FILE_NAME_SIZE
 */
int sm_initialize(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE]);

/** \brief retrieves a new chunk of data.
 *  
 *  Operation carried out by worker thread.
 *  Then calling this function the worker is given and fileHandler identifying the working file. The size
 *  of the chunk of data is always less or equal then DATA_BUFFER_SIZE. If there's no more text to process
 *  no chunk of data is retrieved and this function returns false, the thread might end is execution.
 *
 *  \param id Worker thread id
 *  \param[out] data Buffer containing the chunk of Data
 *  \param[out] size The size of the chunk of Data
 *  \param[out] fileHandler Target processing file handler
 *
 *  \returns true If a new chunk was retrieve, otherwise false
 *
 *  \sa DATA_BUFFER_SIZE
 */
bool sm_getChunkOfData(int id, unsigned char data[DATA_BUFFER_SIZE], unsigned int *size, FileHandler *fileHandler);

/** \brief Registers the results of a file's chunk of data
 *  
 *  Operation carried out by worker thread.
 *  After processing a chunk of data the worker calls this function to register the results.
 * 
 *  \param id Worker thread id
 *  \param fileHandler Target processing file handler
 *  \param count List of counting results
 */
void sm_registerResult(int id, FileHandler fileHandler, Count *count);

/** \brief Retrieve final results
 * 
 *  Operation carried out by main thread
 * 
 *  \param[out] results Array containing the results of each file 
 */
void sm_getResults(Results *results);

/** \brief Close shared memory
 * 
 *  Operation carried out by main thread
 *  Free shared memory resources.
 *  
 *  \returns FAILURE If an error occurs, otherwise SUCCESS
 *  \sa FAILURE
 *  \sa SUCCESS 
 */
int sm_close();

#endif /* SHARED_MEMORY_H */
