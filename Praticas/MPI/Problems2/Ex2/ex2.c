#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
  int rank, size;
  int n = 20;
  double a[n][n];
  double b[n][n];
  double c[n][n];

  int disp[size];
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  MPI_Scatterv (sendData, sendCount, disp, MPI_INT, recData, recCount, MPI_INT, 0,
MPI_COMM_WORLD);

  MPI_Finalize ();
  return EXIT_SUCCESS;
}
