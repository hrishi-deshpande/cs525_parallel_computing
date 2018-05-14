#include <iostream>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

using namespace std;
int main(int argc, char **argv)
{
	uint64_t n, i = 0, j = 0;

	if (argc != 3) {
		cout <<"Usage: ./a.out <matrix_size> <1/2 1-row-wise product, 0-column-wise product>\n";
		exit(1);
	}
	n = atoll(argv[1]);
	int row = atoi(argv[2]);

	float** mat = new float*[n];

	for(int i = 0; i < n; i++) {
	    mat[i] = new float[n];
	}

	float *vector = new float[n];

	float *prod = new float[n];

	float temp = 0;

	srand((unsigned)time(NULL));
	for (i = 0; i < n; i++) {
	    temp = rand();
	    if (temp != 0) {
		    vector[i] = (float)RAND_MAX / temp;
	    } else {
	    	    vector[i] = temp;
	    }
	    for ( j = 0; j < n; j++) {
	    	 temp = rand();
		 if (temp != 0) {
			mat[i][j] = (float)RAND_MAX/temp;
		 } else {
			mat[i][j] = (float)rand();
		 }
	    }
	}


	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);

		
	if (row == 1) {
	    for (i = 0; i < n; i++) {
		for ( j = 0; j < n; j++) {
			prod[i] += mat[i][j]*vector[j];
		}
	    }
	} else {
	    for (j = 0; j < n; j++) {
		for ( i = 0; i < n; i++) {
			prod[i] += mat[i][j]*vector[j];
		}
	    }
	}
	gettimeofday(&end_time, NULL);

	double elapsed_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - 
	     			    (start_time.tv_sec*1000000 + start_time.tv_usec);
	     
	double elapsed_time = elapsed_time_usecs/1000000;

	long double bandwidth = 2 * n/elapsed_time * n;

        cout << fixed << "\nTime for computation: " << elapsed_time << " secs \nSpeed: " << bandwidth << " FLOPS\n";

	for (i = 0; i < n; i++) {
	     delete [] mat[i];
	} 

	delete []  mat; 
	delete [] vector;
	delete [] prod; 

	return 0;
}
