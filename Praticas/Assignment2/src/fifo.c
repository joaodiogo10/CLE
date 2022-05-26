#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "fifo.h"

/**
 *  \file fifo.c implementation
 *
 *  \brief Fifo that store data chunks to be processed.
 *
 *  \author João Diogo Ferreira, João Tiago Rainho - May 2022
 */

/** \brief reading thread return status */
extern int statusReadingThread;

/** \brief proxy threads return status */
extern int * statusProxyThread;

/** \brief storage region */
static Chunk * mem[FIFO_MAX_SIZE];

/** \brief insertion pointer */
static unsigned int ii;

/** \brief retrieval pointer */
static unsigned int ri;

/** \brief flag signaling the data transfer region is full */
static bool full;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief flag which warrants that the data transfer region is initialized exactly once */
static pthread_once_t init = PTHREAD_ONCE_INIT;;

/** \brief producers synchronization point when the data transfer region is full */
static pthread_cond_t fifoFull;

/** \brief consumers synchronization point when the data transfer region is empty */
static pthread_cond_t fifoEmpty;

/** \brief Signals when all the data chunks have been read */
static bool done = false;

/**
 *  \brief Initialization of the data transfer region.
 *
 *  Internal monitor operation.
 */

static void initialization (void)
{
                                                                                   /* initialize FIFO in empty state */
  ii = ri = 0;                                        /* FIFO insertion and retrieval pointers set to the same value */
  full = false;                                                                                  /* FIFO is not full */

  pthread_cond_init (&fifoFull, NULL);                                 /* initialize producers synchronization point */
  pthread_cond_init (&fifoEmpty, NULL);                                /* initialize consumers synchronization point */
}



void doneReading() {
  pthread_mutex_lock (&accessCR);
  done = true;
  pthread_cond_broadcast (&fifoEmpty);
  pthread_mutex_unlock (&accessCR);
}

void putChunk(Chunk * data)
{
  if ((statusReadingThread = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusReadingThread;                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusReadingThread = EXIT_FAILURE;
       pthread_exit (&statusReadingThread);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */
  

  while (full)                                                           /* wait if the data transfer region is full */
  { if ((statusReadingThread = pthread_cond_wait (&fifoFull, &accessCR)) != 0)
       { errno = statusReadingThread;                                                          /* save error in errno */
         perror ("error on waiting in fifoFull");
         statusReadingThread = EXIT_FAILURE;
         pthread_exit (&statusReadingThread);
       }
  }

  mem[ii] = data;
  ii = (ii + 1) % FIFO_MAX_SIZE;
  full = (ii == ri);

  if ((statusReadingThread = pthread_cond_signal (&fifoEmpty)) != 0)      /* let a consumer know that a value has been
                                                                                                               stored */
     { errno = statusReadingThread;                                                             /* save error in errno */
       perror ("error on signaling in fifoEmpty");
       statusReadingThread = EXIT_FAILURE;
       pthread_exit (&statusReadingThread);
     }

  if ((statusReadingThread = pthread_mutex_unlock (&accessCR)) != 0)                                  /* exit monitor */
     { errno = statusReadingThread;                                                            /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusReadingThread = EXIT_FAILURE;
       pthread_exit (&statusReadingThread);
     }
}

bool getChunk(unsigned int proxyId, Chunk **data)
{

  if ((statusProxyThread[proxyId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusProxyThread[proxyId];                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusProxyThread[proxyId] = EXIT_FAILURE;
       pthread_exit (&statusProxyThread[proxyId]);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  while ((ii == ri) && !full)                                           /* wait if the data transfer region is empty */
  { 
    if(done) {
      pthread_mutex_unlock (&accessCR);
      return false;
    }
    if ((statusProxyThread[proxyId] = pthread_cond_wait (&fifoEmpty, &accessCR)) != 0)
       {
         errno = statusProxyThread[proxyId];                                                          /* save error in errno */
         perror ("error on waiting in fifoEmpty");
         statusProxyThread[proxyId] = EXIT_FAILURE;
         pthread_exit (&statusProxyThread[proxyId]);
       }
  }

  *data = mem[ri];                                                                   /* retrieve a  value from the FIFO */
  ri = (ri + 1) % FIFO_MAX_SIZE;
  full = false;

  if ((statusProxyThread[proxyId] = pthread_cond_signal (&fifoFull)) != 0)       /* let a producer know that a value has been
                                                                                                            retrieved */
     { errno = statusProxyThread[proxyId];                                                             /* save error in errno */
       perror ("error on signaling in fifoFull");
       statusProxyThread[proxyId] = EXIT_FAILURE;
       pthread_exit (&statusProxyThread[proxyId]);
     }

  if ((statusProxyThread[proxyId] = pthread_mutex_unlock (&accessCR)) != 0)                                   /* exit monitor */
     { errno = statusProxyThread[proxyId];                                                             /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusProxyThread[proxyId] = EXIT_FAILURE;
       pthread_exit (&statusProxyThread[proxyId]);
     }

  return true;
}
