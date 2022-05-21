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

  recData = (char *)malloc(100);
  sprintf(data, "I am okay! says %d\n", rank);

  if(rank == 0)
  {
    for(int i = 1; i < size; i++)
    {
      printf("%s", data);
      MPI_Send(data, strlen(data), MPI_CHAR, i, 0, MPI_COMM_WORLD);

      memset((void*)recData, '\0', 50);
      MPI_Recv(recData, 50, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%s", recData);
    }
  }
  else
  {
    MPI_Recv(recData, 50, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(data, strlen(data), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  }

  free(recData);
  MPI_Finalize();

  return EXIT_SUCCESS;
}
