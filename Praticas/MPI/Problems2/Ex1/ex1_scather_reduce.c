#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_VALUES 16

int main (int argc, char *argv[])
{
  int rank, size;
  int maxValue;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  
  
  int values[NUMBER_OF_VALUES];
  
  if(rank == 0)
  {
    printf ("Running with %d process\n", size);
    printf ("Generated values: \n");
    for(int i = 0; i < NUMBER_OF_VALUES; i++)
    {
      values[i] = ((double) rand () / RAND_MAX) * 1000;
      printf("%d ", values[i]);
    }
    printf("\n");
  }

  //scather 4
  int recDataScather[4];
  MPI_Scatter(values, 4, MPI_INT, recDataScather, 4, MPI_INT, 0, MPI_COMM_WORLD);

  //reduce 4
  int recDataReduce[4];
  MPI_Reduce(recDataScather, recDataReduce, 4, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  //scather 1
  int recDataScather2;
  MPI_Scatter(recDataReduce, 1, MPI_INT, &recDataScather2, 1, MPI_INT, 0, MPI_COMM_WORLD);

  //reduce 1
  int recDataReduce2;
  MPI_Reduce(&recDataScather2, &recDataReduce2, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  maxValue = recDataReduce2;

  if (rank == 0)
    printf ("Highest value = %d\n", maxValue);

  MPI_Finalize ();
  return EXIT_SUCCESS;
}
