#ifndef TEXT_FILES_H
#define TEXT_FILES_H

#include <stdbool.h>
#include <stdint.h>
#include "probConst.h"

/** \brief Operation successful return code */
#define SUCCESS 1

/** \brief Operation failure return code */
#define FAILURE 0

/** \brief Opaque FileHandler used by workers to identify the target file */
typedef struct sFileHandler *FileHandler;

typedef uint32_t Result[3]; /*!< Number of words ending in consoant */
                            /*!< Number of words beginning in vowel */
                            /*!< Total number of words */

int tf_initialize(int nFiles, char files[nFiles][MAX_FILE_NAME_SIZE]);

int tf_close();

int tf_readChunk(unsigned char data[DATA_BUFFER_SIZE], FileHandler *fileHandler, bool* moreWork);

void tf_registerResult(FileHandler fileHandler, Result count);

int tf_getResults(Result *results);

#endif /* TEXT_FILES_H */
