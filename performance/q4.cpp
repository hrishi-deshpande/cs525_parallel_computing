#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

int main(int argc, char **argv)
{
	uint64_t n, i = 0, j = 0, k = 0;

	if (argc != 2) {
		cout << "Usage ./a.out <matrix_size>\n";
		exit(1);
	}

	n = atoll(argv[1]);

 	float *mat1 = new float[n*n];

	float *mat2 = new float[n*n];

	float *mat3 = new float[n*n];

	float temp = 0;

	srand((unsigned)time(NULL));
	for (i = 0; i < n; i++) {
	    for ( j = 0; j < n; j++) {
	    	 temp = rand();
		 if (temp != 0) {
			*(mat1 + i*n + j) = (float)RAND_MAX/temp;
			
		 } else {
			*(mat1 + i*n + j) = (float) rand();
		 }
		temp = rand();
		if (temp != 0) {
			*(mat2 + i *n + j) = (float)RAND_MAX/temp;	
		} else {
			*(mat2 + i*n + j) = (float)rand();
		}
	    }
	}
	
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			for (k = 0; k < n; k++) {
				*(mat3 + i*n + j) += *(mat1 + i*n + k) * *(mat2 + k*n +j);
			}
		}
	}

	gettimeofday(&end_time, NULL);

	double elapsed_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - 
	     				 (start_time.tv_sec*1000000 + start_time.tv_usec);
	     
	double elapsed_time_secs = elapsed_time_usecs/1000000;

	long double bandwidth = 2 * n * n/elapsed_time_secs * n;

        cout << fixed << "\nTime for computation: " << elapsed_time_secs << " secs\n Speed: " << bandwidth << " FLOPS\n";

	delete [] mat1;
	delete [] mat2;
	delete [] mat3;

	return 0;
}
