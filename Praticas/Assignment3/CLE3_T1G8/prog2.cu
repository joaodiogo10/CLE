#include "common.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <unistd.h>

// (row, col, order)
#define idx(x,y,order)(x*order+y)

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"

inline void switch_row(double *matrix, int row1, int row2, int order);

void determinantOnHostColumns(double *matrices, int numberOfMatrix, double *determinant, int order);

__global__ void determinantOnGPUColumns(double *mat, double *determinant, int order);

void checkResult(double *cpuRef, double *gpuRef, int nDeterminants);

static double get_delta_time(void);

int main(int argc, char **argv)
{
    printf("%s Starting...\n", argv[0]);

    // process cli 
    int opt;
    char * fileName;

    do {
        switch((opt = getopt(argc, argv, "f:h"))) {
            case 'f':
                fileName = optarg;
                break;
                
            case 'h':
                printf("-f      --- filename\n");
                break;
        }
    }
    while(opt != -1);

    // set up device
    int dev = 0;
    cudaDeviceProp deviceProp;
    CHECK(cudaGetDeviceProperties(&deviceProp, dev));
    printf("Using Device %d: %s\n", dev, deviceProp.name);
    CHECK(cudaSetDevice(dev));

    // set up data size of matrix
    int order;
    int numberOfMatrix;
    
    FILE * ptrFile = fopen(fileName, "r"); 
    if(ptrFile == NULL) {
        fprintf(stderr,"Error opening file");
        printf("%s\n", fileName);
        exit(EXIT_FAILURE);
    }     
    size_t size = fread(&numberOfMatrix, sizeof(unsigned int), 1, ptrFile);
    if(size != 1)
    {
        fprintf(stderr,"Error reading the number of matrix in the file\n");
        exit(EXIT_FAILURE);
    }

    size = fread(&order, sizeof(unsigned int), 1, ptrFile);
    if(size != 1)
    {
        fprintf(stderr,"Error reading order from file\n");
        exit(EXIT_FAILURE);
    }

    printf("Filename: %s\nNumber of matrices: %d\nMatrices order: %d\n", fileName, numberOfMatrix, order);

    int nBytesMatrices = order * order * numberOfMatrix * sizeof(double);
    int nBytesDeterminants = numberOfMatrix * sizeof(double);
    
    if ((nBytesMatrices + nBytesDeterminants) > (size_t) 5e9)
    { 
        fprintf (stderr,"The GeForce GTX 1660 Ti cannot handle more than 5GB of memory!\n");
        exit(EXIT_FAILURE);
    }
    
    printf ("Total matrices data size: %d\n", nBytesMatrices);
    printf ("Total determinants data size: %d\n", nBytesDeterminants);

    //host memory
    double determinantRefCPU[numberOfMatrix];
    double determinantRefGPU[numberOfMatrix];
    double *h_matrices = (double *)malloc(nBytesMatrices);
    size = fread(h_matrices, sizeof(double), order * order * numberOfMatrix, ptrFile);
    if(size != order * order * numberOfMatrix)
    {
        fprintf(stderr,"Error matrices from file\n");
        exit(EXIT_FAILURE);
    }


    // malloc device global memory
    double *d_matrices;
    double *d_determinant;

    CHECK(cudaMalloc((void **)&d_matrices, nBytesMatrices));
    CHECK(cudaMalloc((void **)&d_determinant, nBytesDeterminants));

    // transfer data from host to device
    (void) get_delta_time();
    CHECK(cudaMemcpy(d_matrices, h_matrices, nBytesMatrices, cudaMemcpyHostToDevice));
    printf ("The transfer of %d bytes from the host to the device took %.3e seconds\n",
            nBytesMatrices + nBytesDeterminants, get_delta_time());

    // calculate determinant at host side
    (void) get_delta_time();
    determinantOnHostColumns(h_matrices, numberOfMatrix, determinantRefCPU, order);
    printf("The cpu kernel took %.3e seconds to run (single core)\n", get_delta_time ());

    unsigned int gridDimX, gridDimY, blockDimX, blockDimY;

    // invoke kernel at host side
    blockDimX = order;
    blockDimY = 1 << 0;
    gridDimX = numberOfMatrix;
    gridDimY = 1 << 0;
    dim3 block(blockDimX, blockDimY);
    dim3 grid(gridDimX, gridDimY);

    (void) get_delta_time ();
    determinantOnGPUColumns<<<grid, block, order * sizeof(double)>>>(d_matrices, d_determinant, order);
    CHECK(cudaDeviceSynchronize());
    CHECK(cudaGetLastError());
    printf("determinantOnGPUCols <<<(%d,%d), (%d,%d)>>> elapsed %.3e sec\n", grid.x,
           grid.y,
           block.x, block.y, get_delta_time ());


    // copy kernel result back to host side
    CHECK(cudaMemcpy(determinantRefGPU, d_determinant, nBytesDeterminants, cudaMemcpyDeviceToHost));

    // check device results
    checkResult(determinantRefCPU, determinantRefGPU, numberOfMatrix);

    // free device global memory
    CHECK(cudaFree(d_matrices));
    CHECK(cudaFree(d_determinant));

    // reset device
    CHECK(cudaDeviceReset());

    return (0);
}

inline void switch_row(double *matrix, int row1, int row2, int order) {
    double aux;
    
    for(int i = 0; i < order; i++) 
    {
        aux = matrix[idx(row1,i,order)];
        matrix[idx(row1,i,order)] = matrix[idx(row2,i,order)];
        matrix[idx(row2,i,order)] = aux;
    }
}

void determinantOnHostColumns(double *matrices, int numberOfMatrix, double *determinant, int order)
{
    int sign = 1;
    double ratio = 1;
    
    for(int n = 0; n < numberOfMatrix; n++)
    {
        double *matrix = &matrices[n * order * order];
        determinant[n] = 1;
        
        // for each row
        for(int i = 0; i < order; i++) 
        {
            // check if the row can be used, otherwise, switch that row
            if(matrix[idx(i,i,order)] == 0) {
                bool determinantIsZero = true;
                for(int j = i+1; j < order; j++) 
                {
                    if(matrix[idx(j,i,order)] != 0) 
                    {
                        switch_row(matrix, i, j, order);
                        sign = (sign == 1) ? -1: 1;
                        determinantIsZero = false;
                        break;
                    }
                }
                if(determinantIsZero)
                {
                    determinant[n] = 0;
                    break;
                }                       
            }

            // for all other rows
            for(int j = i + 1; j < order; j++) {
                ratio = matrix[idx(j,i,order)] / matrix[idx(i,i,order)];
                for(int k = 0; k < order; k++) {
                    matrix[idx(j,k,order)] = matrix[idx(j,k,order)] - ratio * matrix[idx(i,k,order)];
                }
            }
            determinant[n] *= matrix[idx(i,i,order)];
        }
        determinant[n] *= sign;
    }
}

__global__ void determinantOnGPUColumns(double *mat, double *determinant, int order)
{
    extern __shared__ double tmp[];  

    unsigned int columnNumber = threadIdx.x + threadIdx.y * blockDim.x;
    unsigned int matrixNumber = blockIdx.x + blockIdx.y * gridDim.x;
    unsigned int size = order * order;

    double *matrix = &mat[matrixNumber * size];

    int sign = 1;
    determinant[matrixNumber] = 1;
    
    //for each row
    for(int i = 0; i < order; i++)
    {
        //swap row if necessary
        if(matrix[idx(i,i,order)] == 0) {
            bool determinantIsZero = true;
            for(int j= i + 1; j < order; j++) {
                if(matrix[idx(j,i,order)] != 0) {
                    double t = matrix[idx(i,columnNumber,order)];
                    matrix[idx(i,columnNumber,order)] = matrix[idx(j,columnNumber,order)];
                    matrix[idx(j,columnNumber,order)] = t;
                    sign = (sign == 1) ? -1 : 1;
                    determinantIsZero = false;

                    break;
                }
            } 
            //determinant is 0
            if(determinantIsZero)
            {            
                if(columnNumber == 0)
                    determinant[matrixNumber] = 0;
                return;               
            }
        }
        __syncthreads();

        //Read all necessary values
        tmp[columnNumber] = matrix[idx(columnNumber,i,order)];
        __syncthreads();

        //For all other rows
        for(int j = i + 1; j < order; j++)
        {
            double ratio = tmp[j] / matrix[idx(i,i,order)];

            //process corresponding col
            matrix[idx(j,columnNumber,order)] = matrix[idx(j,columnNumber,order)]-ratio*matrix[idx(i,columnNumber,order)];
        }
        __syncthreads();
    }

    //calculate determinant
    if(columnNumber == 0)
    {
        determinant[matrixNumber] = sign;
        for(int i = 0; i < order; i++)
            determinant[matrixNumber] = determinant[matrixNumber] * matrix[idx(i,i,order)];
    }
}

void checkResult(double *cpuRef, double *gpuRef, int nDeterminants)
{
    bool match = 1;
    for(int i = 0; i < nDeterminants; i++)
    {
        double epsilon = (1 - cpuRef[i] / gpuRef[i]) * 100;
        if(epsilon < 0)
            epsilon = -epsilon;

        if (epsilon > 0.00001)
        {
            match = 0;
            printf("%sError: Matrix %3d - host %.8e \t gpu %.8e\n%s", KRED, i + 1, cpuRef[i], gpuRef[i], KNRM);
            break;
        }

        printf("%sCorrect: Matrix %3d - host %.3e \t gpu %.3e\n%s", KGRN, i + 1, cpuRef[i], gpuRef[i], KNRM);
    }

    if (match)
        printf("Determinants match.\n\n");
    else
        printf("Determinants do not match.\n\n");
}

static double get_delta_time(void)
{
  static struct timespec t0,t1;

  t0 = t1;
  if(clock_gettime(CLOCK_MONOTONIC,&t1) != 0)
  {
    perror("clock_gettime");
    exit(1);
  }
  return (double)(t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}