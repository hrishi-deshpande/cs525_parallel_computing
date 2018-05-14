#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <float.h>

using namespace std;
int main(int argc, char **argv) 
{
	if (argc != 2) {
		cout << "Usage: ./a.out <array_size>\n";
		exit(1);
	}

	uint64_t size = 0;

	size = atoll(argv[1]);

	cout <<"Array Size: " << size << "\n";

	float *arr = new float[size];

	uint64_t i = 0;

	srand((unsigned)time(NULL));
	for ( i = 0; i < size; i++) {
		float temp = rand();
		if (temp != 0 ) {
			arr[i] = (RAND_MAX/temp);
		} else {
			arr[i] = (float) rand()/1.0001;
		}
	}

	long double sum = -LDBL_MAX;

	struct timeval start_time, end_time;

	gettimeofday(&start_time, NULL);

	for (i = 0; i < size; i++) {
	     sum = sum + arr[i];	     
	}
	
	gettimeofday(&end_time, NULL);

	double elapsed_time_usecs = (end_time.tv_sec * 1000000 + end_time.tv_usec) - 
				      (start_time.tv_sec * 1000000 + start_time.tv_usec);

	double elapsed_time = elapsed_time_usecs/1000000.0;

	setprecision(4);

	double bandwidth = ((double)sizeof(float) * size)/elapsed_time;

	double mb_bw = bandwidth / (1024 * 1024); 

	cout << "Elapsed time in secs: " << elapsed_time << "\n";
	cout << "Bandwidth of the system in MB/s: " << mb_bw << " MB/s\n";

	delete [] arr;
	
	return 0;
}
