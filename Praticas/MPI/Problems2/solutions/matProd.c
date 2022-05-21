#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#define  N      8

int main (int argc, char *argv[])
{
   int rank, nProc;
   int *storeA = NULL, **matA, *storeB, **matB, *storeC = NULL, **matC = NULL;
   int *storeAParc, *storeCParc, **matAParc, **matCParc;
   int n, i, j, k;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);
   MPI_Comm_size (MPI_COMM_WORLD, &nProc);
   if ((nProc <= 1) || ((N % nProc) != 0))
      { if (rank == 0) printf ("Wrong number of processes! It must be a submultiple of %d different from 1.\n", N);
        MPI_Finalize ();
        return EXIT_FAILURE;
      }
   n = N / nProc;
   storeB = malloc (N * N * sizeof (int));
   matB = malloc (N * sizeof (int *));
   storeAParc = malloc (n * N * sizeof (int));
   matAParc = malloc (n * sizeof (int *));
   storeCParc = malloc (n * N * sizeof (int));
   matCParc = malloc (n * sizeof (int *));
   for (i = 0; i < N; i++)
   { matB[i] = ((int *) storeB + i * N);
     if (i < n)
        { matAParc[i] = ((int *) storeAParc + i * N);
          matCParc[i] = ((int *) storeCParc + i * N);
        }
   }
   if (rank == 0)
      { srandom (getpid ());
        storeA = malloc (N * N * sizeof (int));
        storeC = malloc (N * N * sizeof (int));
        matA = malloc (N * sizeof (int *));
        matC = malloc (N * sizeof (int *));
        for (i = 0; i < N; i++)
        { matA[i] = ((int *) storeA + i * N);
          matC[i] = ((int *) storeC + i * N);
        }
        printf ("Generated matrix A values\n");
        for (i = 0; i < N; i++)
        { for (j = 0; j < N; j++)
          { matA[i][j] = ((double) rand () / RAND_MAX - 0.5) * 10;
            printf ("%4d ", matA[i][j]);
          }
          printf ("\n");
        }
        printf ("Generated matrix B values\n");
        for (i = 0; i < N; i++)
        { for (j = 0; j < N; j++)
          { matB[i][j] = ((double) rand () / RAND_MAX - 0.5) * 10;
            printf ("%4d ", matB[i][j]);
          }
          printf ("\n");
        }
      }
   MPI_Bcast(storeB, N * N, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Scatter (storeA, n * N, MPI_INT, storeAParc, n * N, MPI_INT, 0, MPI_COMM_WORLD);
   for (i = 0; i < n; i++)
     for (j = 0; j < N; j++)
     { matCParc[i][j] = 0;
       for (k = 0; k < N; k++)
         matCParc[i][j] += matAParc[i][k] * matB[k][j];
     }
   MPI_Gather (storeCParc, n * N, MPI_INT, storeC, n * N, MPI_INT, 0, MPI_COMM_WORLD);
   if (rank == 0)
   { printf ("Matrix c values\n");
     for (i = 0; i < N; i++)
     { for (j = 0; j < N; j++)
         printf ("%4d ", matC[i][j]);
       printf ("\n");
     }
   }
   MPI_Finalize ();

   return EXIT_SUCCESS;
}
