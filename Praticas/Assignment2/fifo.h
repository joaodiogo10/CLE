#ifndef FIFO_H
#define FIFO_H

#include <stdio.h>
#include <stdint.h>
#include "probConst.h"
#include "textFiles.h"

/** \brief Data chunk process by a worker a respective file handler */
struct sChunk
{
    uint8_t data[DATA_BUFFER_SIZE];
    FileHandler handler;
    Result result; /*!< Number of words ending in consoant */
                   /*!< Number of words beginning in vowel */
                   /*!< Total number of words */
};
typedef struct sChunk Chunk;

/** \brief Inserts a data chunk into fifo
 *  
 *  \param[in] data array containing a chunk of data.
 */
extern void putChunk(Chunk * data);


/** \brief Fetches a data chunk from the fifo
 *  
 *  \param[out] data chunk of data pinter.
 *  \param[in] proxyId Proxy thread id. 
 *  \return True if there are more chunk of data to retrieve. False otherwise.
 */
extern bool getChunk(unsigned int proxyId, Chunk **data);

/** \brief Indication that all files have been read.
 *  
 *  Called by the reading thread to signal dispatcher that there is no more chunks of data.
 */
extern void doneReading();

#endif /* FIFO_H */
