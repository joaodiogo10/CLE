#ifndef TEXT_FILES_H
#define TEXT_FILES_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "probConst.h"
#include "textFiles.h"

/**
 *  \file textFiles.h
 *
 *  \brief Text files header
 *  
 *  This code is responsible to fetch chunks from the text files,
 *  keeping track of the current file being processed and to register
 *  the results of each file.
 *  \author João Diogo Ferreira, João Tiago Rainho - May 2022
 */

/** \brief Operation successful return code */
#define SUCCESS 1

/** \brief Operation failure return code */
#define FAILURE 0

/** \brief Opaque FileHandler used to identify the target file */
typedef struct sFileHandler *FileHandler;

/** \brief Represents the results of a file/chunk processing
 * Result[0] -> Number of words ending in consoant.
 * Result[1] -> Number of words beginning in vowel.
 * Result[2] -> Total number of words.
*/
typedef uint32_t Result[3]; 

/** \brief Text files initialization.
 *  
 *  This function initializes text files resources and it must be called before calling any other
 *  function.
 *
 *  \param nFiles Total number of files
 *  \param files Array containing the names of all files
 *
 *  \returns FAILURE If an error occurs, otherwise SUCCESS
 *  \sa FAILURE
 *  \sa SUCCESS
 *  \sa MAX_FILE_NAME_SIZE
 */
int tf_initialize(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE]);

/** \brief Close the text files.
 * 
 *  Free text files resources.
 *  
 *  \returns FAILURE If an error occurs, otherwise SUCCESS
 *  \sa FAILURE
 *  \sa SUCCESS 
 */
int tf_close();

/** \brief retrieves a new chunk of data.
 *  
 *  The size of the chunk of data is always less or equal then DATA_BUFFER_SIZE - 2. If there's no more text to process
 *  no chunk of data is retrieved and this function returns false, the thread might end is execution. The size of the chunk
 *  is store in the last 2 bytes of the array. 
 * 
 *  data[DATA_BUFFER_SIZE - 1] is the MSB.
 * 
 *  data[DATA_BUFFER_SIZE - 2] is the LSB.
 * 
 *  \param[out] data Buffer containing the chunk of Data
 *  \param[out] fileHandler Target processing file handler
 *  \param[out] moreWork True if there is more dataChunks. False, otherwise.
 *
 *  \returns FAILURE If an error occurs, otherwise SUCCESS
 *  \sa FAILURE
 *  \sa SUCCESS 
 *  \sa DATA_BUFFER_SIZE
 */
int tf_readChunk(uint8_t data[DATA_BUFFER_SIZE], FileHandler *fileHandler, bool* moreWork);

/** \brief Registers the results of a file's chunk of data
 *  
 *  \param fileHandler Target processing file handler
 *  \param count List of counting results
 */
void tf_registerResult(FileHandler fileHandler, Result count);

/** \brief Retrieve final results.
 * 
 *  \param[out] results Array containing the results of each file 
 */
int tf_getResults(Result *results);

#endif /* TEXT_FILES_H */
