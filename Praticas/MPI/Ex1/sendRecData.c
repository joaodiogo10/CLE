#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  int rank, size;
  char data[50],
       *recData;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  if(rank == 0)
  {
    int source = size - 1;
    int destination = (size == 1 ? 0 : 1);

    sprintf(data, "I am here %d!\n", rank);
    MPI_Send(data, strlen(data), MPI_CHAR, destination, 0, MPI_COMM_WORLD);

    recData = (char *)calloc(100, sizeof(char));
    MPI_Recv(recData, 100, MPI_CHAR, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("%s", recData);
  }
  else
  {
    int source = rank - 1;
    int destination = (rank + 1) % size;

    recData = (char *)calloc(100, sizeof(char));
    MPI_Recv(recData, 100, MPI_CHAR, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("%s", recData);

    sprintf(data, "I am here %d!\n", rank);
    MPI_Send(data, strlen(data), MPI_CHAR, destination, 0, MPI_COMM_WORLD);
  }

  free(recData);
  MPI_Finalize();

  return EXIT_SUCCESS;
}
