#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <float.h>

using namespace std;

int main(int argc, char **argv)
{
	uint64_t c = 10000000;

	if (argc != 2) {
	    cout <<"Usage ./a.out <spacing>\n";
	    exit(1);
	}
	uint32_t spacing = atoi(argv[1]);

	uint64_t size = c * spacing;
	cout << "\nExecuting for spacing: " << spacing << " Array Size: " << size << "\n";
	
	float *arr = new float[size];
	float temp = 0;
	uint64_t i = 0, j = 0;

	//Initialize the array with random values.
	for ( i = 0; i < size; i++) {
	      temp = rand();
	      if (temp != 0 ) {
	  	  arr[i] = (float) (RAND_MAX/temp);
	      } else {
		  arr[i] = (float) rand();
	      }
	 }

	 long double sum = -LDBL_MAX;
	 struct timeval start_time, end_time;

	 gettimeofday(&start_time, NULL);
	 for (j = 0; j < size; j = j + spacing) {
	      sum = sum + arr[j];
	 }
	 
	 gettimeofday(&end_time, NULL);

	 double elapsed_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - 
	 	  		     (start_time.tv_sec*1000000 + start_time.tv_usec);
	     
         double elapsed_time_secs = elapsed_time_usecs/1000000;

	 double bandwidth = (sizeof(float)*c)/elapsed_time_secs;
	 double mb_bw = bandwidth/(1024*1024);

	 cout <<"\nTime for computation: " << elapsed_time_secs << " secs \nBandwidth: " << mb_bw << " MB/s\n";

	 delete[] arr;
	
	 return 0;
}
