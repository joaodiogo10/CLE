#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
   int rank, size;
   char *sndData,
        *recData;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   MPI_Comm_size (MPI_COMM_WORLD, &size);
   if (rank == 0)
      { int i;
        MPI_Request req;
        MPI_Status stat;

        sndData = malloc (100);
        recData = malloc (100);
        sprintf (sndData, "I am alive (%d)!", rank);
        printf ("I, %d, am going to transmit the message: %s to all other processes in the group\n", rank, sndData);
        for (i = (rank + 1) % size; i < size; i++)
        { MPI_Isend (sndData, strlen (sndData) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &req);
          MPI_Recv (recData, 100, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          printf ("I, %d, received the message: %s\n", rank, recData);
          MPI_Wait (&req, &stat);
        }
      }
      else { recData = malloc (100);
             MPI_Recv (recData, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
             sndData = malloc (100);
             sprintf (sndData, "I am alive (%d)!", rank);
             MPI_Send (sndData, strlen (sndData) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
           }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}
