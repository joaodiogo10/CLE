#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>

/** sequence length */
#define  N       32

int main (int argc, char *argv[])
{
   int rank, nProc;
   int *seqValMin, *seqValMax, *seqValParcMin, *seqValParcMax;
   bool goOn;
   int i, n, nNorm;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   MPI_Comm_size (MPI_COMM_WORLD, &nProc);
   if (nProc <= 1)
      { if (rank == 0) printf ("Wrong number of processes! It must be greater than 1.\n");
        MPI_Finalize ();
        return EXIT_FAILURE;
      }
   nNorm = N + (((N % nProc) == 0) ? 0 : nProc - (N % nProc));
   seqValMin = malloc (nNorm * sizeof (int));
   seqValMax = malloc (nNorm * sizeof (int));
   if (rank == 0)
      { srandom (getpid ());
        printf ("Generated sequence of values\n");
        for (i = 0; i < nNorm; i++)
          if (i < N)
             { seqValMin[i] = seqValMax[i] = ((double) rand () / RAND_MAX) * 1000;
               printf ("%d ", seqValMin[i]);
             }
             else { seqValMin[i] = INT_MAX;
                    seqValMax[i] = INT_MIN;
                  }
        printf ("\n");
      }
   n = nNorm / nProc;
   seqValParcMin = malloc (n * sizeof (int));
   seqValParcMax = malloc (n * sizeof (int));
   goOn = true;
   while (goOn)
   { if (n == 1) goOn = false;
     MPI_Scatter (seqValMin, n, MPI_INT, seqValParcMin, n, MPI_INT, 0, MPI_COMM_WORLD);
     MPI_Reduce (seqValParcMin, seqValMin, n, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
     MPI_Scatter (seqValMax, n, MPI_INT, seqValParcMax, n, MPI_INT, 0, MPI_COMM_WORLD);
     MPI_Reduce (seqValParcMax, seqValMax, n, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
     nNorm = n + (((n % nProc) == 0) ? 0 : nProc - (n % nProc));
     for (i = 0; i < nNorm; i++)
       if (i >= n)
          { seqValMin[i] = INT_MAX;
            seqValMax[i] = INT_MIN;
          }
     n = nNorm / nProc;
   }
   if (rank == 0)
      { printf ("Smallest value = %d\n", seqValMin[0]);
        printf ("Largest value = %d\n", seqValMax[0]);
      }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}
