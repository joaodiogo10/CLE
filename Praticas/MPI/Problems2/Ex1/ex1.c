#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
  int rank, size;
  int val, maxValue, minValue;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  
  srandom (getpid ());
  val = ((double) rand () / RAND_MAX) * 1000;
  printf ("Value generated by process %d = %d \n", rank, val);
  MPI_Reduce (&val, &maxValue, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce (&val, &minValue, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
  
  if (rank == 0)
  {
    printf ("Highest value = %d\n", maxValue);
    printf ("Lowest value = %d\n", minValue);
  }

  MPI_Finalize ();
  return EXIT_SUCCESS;
}
