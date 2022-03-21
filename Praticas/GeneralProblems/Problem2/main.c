#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

char * filePath = "dataSetProb2b/computeDet/mat128_32.txt";

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "USAGE: ./matrixDeterminant filePath\n");
        return EXIT_FAILURE;
    }

    char *filePath = argv[1];
    FILE *ptrFile;
    ptrFile = fopen(filePath, "rb");
    if (ptrFile == NULL)
    {
        fprintf(stderr, "Error opening file \"%s\"\n", filePath);
        return EXIT_FAILURE;
    }
    
    unsigned int nMatrices;
    fread(&nMatrices, sizeof(unsigned int), 1, ptrFile);
    if (ferror(ptrFile) != 0 || feof(ptrFile))
    {
        fprintf(stderr, "Error: invalid file format\n");
        return EXIT_FAILURE;
    }

    unsigned int order;
    fread(&order, sizeof(unsigned int), 1, ptrFile);
    if (ferror(ptrFile) != 0 || feof(ptrFile))
    {
        fprintf(stderr, "Error: invalid file format\n");
        return 1;
    }

    //start
    printf("Computing %d matrix (%d,%d)...\n", nMatrices, order, order);
    double m[order][order];

    //parse matrix
    for(int row = 0; row < order; row++)
    {
        for(int col = 0; col < order; col++)
        {
            double tmp;
            fread(&tmp, 8, 1, ptrFile);
            if (ferror(ptrFile) != 0 || feof(ptrFile))
            {
                fprintf(stderr, "Error parsing matrix entries, verify file format\n");
                return EXIT_FAILURE;
            }
            m[row][col] = tmp;
        }
    }

    //print matrix
    /*for(int row = 0; row < order; row++)
    {
        printf("\n%d- ", row);
        for(int col = 0; col < order; col++)
        {
            printf("%f ", m[row][col]);
        }
    }*/
    
    for(int matrix = 0; matrix < nMatrices ; matrix++) //for each matrix
    { 
        printf("\n----Computing matrix %d----\n", matrix + 1);
        double determinant = 1;
        unsigned int nSwap = 0;

        //perform gausian elimination
        for(int i = 0; i < order; i++) 
        {
            if(m[i][i] == 0) 
            {  
                bool swapped = false; 
                //find a col, j, whose coefficient m[i][j] != 0, for j > i. Then swap cols
                for(int j = i+1; j < order; j++) 
                {
                    if(m[i][j] != 0)
                    {
                        //swap cols
                        for(int row = 0; row < order; row++)
                        {
                            double tmp = m[row][i]; //col i
                            m[row][i] = m[row][j];  //col i = col j
                            m[row][j] = tmp;        //col j = col i
                        }
                        swapped = true;
                        nSwap += 1;
                        break;
                    }
                }

                           
                //all col's coefficients == 0 => determinant = 0 
                /*if(swapped == false)
                {
                    determinant = 0;
                    break;
                }*/
            }
            //apply transformation a_kj = a_kj - a_ki/a_ii * a_ij
            for(int k = i + 1; k < order; k++) //for all rows, k, below row i
            {   
                for(int j = i; j < order; j++) //for all cols, starting at col i (diagonal)
                {
                    m[k][j] = m[k][j] - m[k][i]/m[i][i] * m[i][j];
                }
            }
            printf("determinant: %e\n", determinant);
            //printf("%e\n", m[i][i]);
            //multiple diagonal
            determinant = determinant * m[i][i];
        }

        //determine determinant signal
        determinant = ( nSwap % 2 == 1) ? -determinant : determinant;

        printf("Determinant: %e\n", determinant);

        break;
    }
    return EXIT_SUCCESS;
}