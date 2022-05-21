#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_VALUES 16

int main(int argc, char *argv[])
{
  int rank, size;
  int maxValue;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int values[NUMBER_OF_VALUES];

  for(int i = 0; i < NUMBER_OF_VALUES; i++)
    values[i] = 0;

  if (rank == 0)
  {
    printf("Running with %d processes\n", size);
    printf("Generated values: \n");
    for (int i = 0; i < NUMBER_OF_VALUES; i++)
    {
      values[i] = ((double)rand() / RAND_MAX) * 1000;
      printf("%d ", values[i]);
    }
    printf("\n");
  }

  // first scather
  int firstScatherSendCount = (NUMBER_OF_VALUES + (size - 1)) / size;
  int firstScatherReceiveCount = firstScatherSendCount;
  int firstScatherReceiveData[firstScatherReceiveCount];

  int sendSize;

  //add padding if necessary
  if(NUMBER_OF_VALUES < size && NUMBER_OF_VALUES % size != 0)
    sendSize = 2 * size;
  else
    sendSize = NUMBER_OF_VALUES;

  int firstScatherSendData[sendSize];
  for(int i = 0; i < sendSize; i++)
    firstScatherSendData[i] = (i < NUMBER_OF_VALUES) ? values[i] : 0;

  MPI_Scatter(firstScatherSendData, firstScatherSendCount, MPI_INT, firstScatherReceiveData, firstScatherReceiveCount, MPI_INT, 0, MPI_COMM_WORLD);

  // first reduce
  int firstReduceReceiveData[firstScatherReceiveCount];
  MPI_Reduce(firstScatherReceiveData, firstReduceReceiveData, firstScatherReceiveCount, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  // second scather
  int secondScatherSendCount = (firstScatherReceiveCount + (size - 1)) / size;
  int secondScatherReceiveCount = secondScatherSendCount;
  int secondScatherReceiveData[secondScatherReceiveCount];
  
  int secondScatherSendData[size];
  for(int i = 0; i < size; i++)
    secondScatherSendData[i] = (i < firstScatherReceiveCount) ? firstReduceReceiveData[i] : 0;

  MPI_Scatter(secondScatherSendData, secondScatherSendCount, MPI_INT, secondScatherReceiveData, secondScatherReceiveCount, MPI_INT, 0, MPI_COMM_WORLD);

  // second reduce
  int sencondReduceReceiveData;
  MPI_Reduce(&secondScatherReceiveData, &sencondReduceReceiveData, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

  maxValue = sencondReduceReceiveData;

  if (rank == 0)
    printf("Highest value = %d\n", maxValue);

  MPI_Finalize();
  return EXIT_SUCCESS;
}
