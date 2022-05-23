#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "fifo.h"

/** \brief producer threads return status array */
extern int statusProd;

/** \brief consumer threads return status array */
extern int statusCons;

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
  if ((statusProd = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusProd;                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusProd = EXIT_FAILURE;
       pthread_exit (&statusProd);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */
  

  while (full)                                                           /* wait if the data transfer region is full */
  { if ((statusProd = pthread_cond_wait (&fifoFull, &accessCR)) != 0)
       { errno = statusProd;                                                          /* save error in errno */
         perror ("error on waiting in fifoFull");
         statusProd = EXIT_FAILURE;
         pthread_exit (&statusProd);
       }
  }

  mem[ii] = data;
  ii = (ii + 1) % FIFO_MAX_SIZE;
  full = (ii == ri);

  if ((statusProd = pthread_cond_signal (&fifoEmpty)) != 0)      /* let a consumer know that a value has been
                                                                                                               stored */
     { errno = statusProd;                                                             /* save error in errno */
       perror ("error on signaling in fifoEmpty");
       statusProd = EXIT_FAILURE;
       pthread_exit (&statusProd);
     }

  if ((statusProd = pthread_mutex_unlock (&accessCR)) != 0)                                  /* exit monitor */
     { errno = statusProd;                                                            /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusProd = EXIT_FAILURE;
       pthread_exit (&statusProd);
     }
}

bool getChunk(Chunk **data)
{

  if ((statusCons = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = statusCons;                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       statusCons = EXIT_FAILURE;
       pthread_exit (&statusCons);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  while ((ii == ri) && !full)                                           /* wait if the data transfer region is empty */
  { 
    if(done) {
      pthread_mutex_unlock (&accessCR);
      return false;
    }
    if ((statusCons = pthread_cond_wait (&fifoEmpty, &accessCR)) != 0)
       {
         errno = statusCons;                                                          /* save error in errno */
         perror ("error on waiting in fifoEmpty");
         statusCons = EXIT_FAILURE;
         pthread_exit (&statusCons);
       }
  }

  *data = mem[ri];                                                                   /* retrieve a  value from the FIFO */
  ri = (ri + 1) % FIFO_MAX_SIZE;
  full = false;

  if ((statusCons = pthread_cond_signal (&fifoFull)) != 0)       /* let a producer know that a value has been
                                                                                                            retrieved */
     { errno = statusCons;                                                             /* save error in errno */
       perror ("error on signaling in fifoFull");
       statusCons = EXIT_FAILURE;
       pthread_exit (&statusCons);
     }

  if ((statusCons = pthread_mutex_unlock (&accessCR)) != 0)                                   /* exit monitor */
     { errno = statusCons;                                                             /* save error in errno */
       perror ("error on exiting monitor(CF)");
       statusCons = EXIT_FAILURE;
       pthread_exit (&statusCons);
     }
  return true;
}
