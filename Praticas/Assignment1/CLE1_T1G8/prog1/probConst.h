#ifndef PROB_CONST_H_
#define PROB_CONST_H_

/**
 *  \file probConst.h
 *
 *  \brief Constants definition header
 *
 *  Problem simulation parameters.
 *
 *  \author João Diogo Ferreira, João Tiago Rainho - April 2022
 */


/** \brief number of workers */
#define N   8

/** \brief maximum file path size */
#define MAX_FILE_NAME_SIZE 50       

/** \brief size of worker's data chuck buffer */
#define DATA_BUFFER_SIZE 2 << 12

#endif /* PROB_CONST_H_ */