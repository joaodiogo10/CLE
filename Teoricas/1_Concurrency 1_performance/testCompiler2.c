#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define N    20000

int x[N],
    y[N];
int a[N][N];

int main (int argv, char *argc[])
{
	int i, j;
	double t0, t1;

	/* array initialization */

	srand ((int) getpid ());
   	for (i = 0; i < N; i++)
	{ x[i] = rand ();
	  y[i] = 0;
      for (j = 0; j < N; j++)
	    a[i][j] = rand ();
	}

   	/* matrix - vector multiplication */

   	t0 = ((double) clock ()) / CLOCKS_PER_SEC;
	for (i = 0; i < N; i++)
   	  for (j = 0; j < N; j++)
	    y[i] += a[i][j] * x[j];
	t1 = ((double) clock ()) / CLOCKS_PER_SEC;
	printf ("Elapsed time = %.6f s\n", t1 - t0);

	return 0;
}
