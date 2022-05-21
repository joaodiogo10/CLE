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
      { sndData = malloc (100);
        sprintf (sndData, "I am here (%d)!", rank);
        printf ("I, %d, am going to transmit the message: %s\n", rank, sndData);
        MPI_Send (sndData, strlen (sndData) + 1, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
        recData = malloc (100);
        MPI_Recv (recData, 100, MPI_CHAR, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf ("I, %d, received the message: %s\n", rank, recData);

      }
      else { recData = malloc (100);
             MPI_Recv (recData, 100, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
             printf ("I, %d, received the message: %s\n", rank, recData);
             sndData = malloc (100);
             sprintf (sndData, "I am here (%d)!", rank);
             printf ("I, %d, am going to transmit the message: %s\n", rank, sndData);
             MPI_Send (sndData, strlen (sndData) + 1, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD);
           }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}
