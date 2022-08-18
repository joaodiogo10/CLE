#ifndef PROB_CONST_H_
#define PROB_CONST_H_

/**
 *  \file probConst.h
 *
 *  \brief Constants definition header
 *
 *  Problem simulation parameters.
 *
 *  \author João Diogo Ferreira, João Tiago Rainho - May 2022
 */


/** \brief number of workers */
#define N   8

/** \brief maximum file path size */
#define MAX_FILE_NAME_SIZE 50       

/** \brief size of worker's data chuck buffer, last 2 bytes of the chunk specifies the amount of data sent */
#define DATA_BUFFER_SIZE ((2 << 12) + 2) 

/** \brief maximum size of fifo */
#define FIFO_MAX_SIZE 20

#endif /* PROB_CONST_H_ */