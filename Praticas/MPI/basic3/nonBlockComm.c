#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int main (int argc, char *argv[])
{
   int rank, size;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   MPI_Comm_size (MPI_COMM_WORLD, &size);
   if (rank == 0)
      { int i;
        char *sndData, **recData;
        bool allMsgRec, recVal, msgRec[size];
        MPI_Request reqSnd[size], reqRec[size];

        sndData = malloc (100);
        recData = malloc (size * sizeof (char *));
        for (i = (rank + 1) % size; i < size; i++)
          recData[i] = malloc (100);
        sprintf (sndData, "Start working!");
        printf ("I, %d, am going to transmit the message: %s to all other processes in the group\n", rank, sndData);
        for (i = (rank + 1) % size; i < size; i++)
          MPI_Isend (sndData, strlen (sndData) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &reqSnd[i]);
        printf ("I, %d, am going to receive a message from all other processes in the group\n", rank);
        for (i = (rank + 1) % size; i < size; i++)
        { MPI_Irecv (recData[i], 100, MPI_CHAR, i, 0, MPI_COMM_WORLD, &reqRec[i]);
          msgRec[i] = false;
        }
        do
        { allMsgRec = true;
          for (i = (rank + 1) % size; i < size; i++)
            if (!msgRec[i])
               { recVal = false;
                 MPI_Test(&reqRec[i], (int *) &recVal, MPI_STATUS_IGNORE);
                 if (recVal)
                    { printf ("I, %d, received the message: %s\n", rank, recData[i]);
                      msgRec[i] = true;
                    }
                    else allMsgRec = false;
               }
        } while (!allMsgRec);
      }
      else { char *sndData, *recData;

             recData = malloc (100);
             MPI_Recv (recData, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
             sndData = malloc (100);
             sprintf (sndData, "I am working (%d)!", rank);
             MPI_Send (sndData, strlen (sndData) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
           }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}
