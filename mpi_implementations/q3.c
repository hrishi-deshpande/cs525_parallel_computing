#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

#define MSG_TAG 1
#define CONS_TAG 2
#define ACK_TAG 3
#define SLEEP_TIME 120

int world_size = 0, my_rank = 0;
int consumed_count  = 0;

void my_terminate_handler() {
	int flag = 0;
	MPI_Request request;
	MPI_Status status;

	uint64_t final_count = 0;

	struct timeval start_time, end_time;

	double time = 5, time_diff = 0;
	int data = 0;

	gettimeofday(&start_time, NULL);

	if (world_size > 2) {
		time *= 2;
	}

	do {
		usleep(1000);

		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
		if (flag != 0) {
			MPI_Recv((void *)&data, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
		}

		gettimeofday(&end_time, NULL);

		time_diff = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec)/1000000;

	} while(time_diff < time);

	MPI_Reduce((void *)&consumed_count, (void *)&final_count, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

	if (my_rank == 0) {
		printf("\nTotal consumed count = %lu\n", final_count);
	}

    	MPI_Finalize();
	exit(0);
}

void check_time(struct timeval start_time) {
	struct timeval end_time;

	gettimeofday(&end_time, NULL);
	
	double time_diff = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec)/1000000;

	if (time_diff < SLEEP_TIME) {
		return;
	}

	my_terminate_handler();
}


int main(int argc, char **argv)
{
    int random_proc = 0;
    int flag = 0, data = 0;

    MPI_Status status, status2;
    MPI_Request request;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 2) {
	printf("\nNo of processes should be atleast 2 for the system to work\n");
	exit(1);
    }

    struct timeval start_time, end_time;
    int half_size = world_size/2;

    if (half_size %2 != 0) half_size++;

    gettimeofday(&start_time, NULL);

    //NOTE: since we have 2 processes per core, one has an even rank and the other an odd rank.
    if (my_rank % 2 != 0) {
        //Let's make processes with odd rank as producers.
	
	while (1) {
	    srand(time(NULL) + my_rank);
	    do {
	    	data = rand();
	        random_proc = (data%half_size) * 2;
	    }while(random_proc%2 != 0);

	    check_time(start_time);
	    
	    MPI_Send((void *)&data, 1, MPI_INT, random_proc, MSG_TAG, MPI_COMM_WORLD);
	    do {
	    	flag = 0;
		MPI_Iprobe(MPI_ANY_SOURCE, ACK_TAG, MPI_COMM_WORLD, &flag, &status);
		check_time(start_time);
	    }while (flag == 0);

	    MPI_Recv((void *)&data, 1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

    } else {
    	//Similarly processes with even rank are consumers.
	while (1) {
		check_time(start_time);
		MPI_Iprobe(MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &flag, &status);
		if (flag != 0) {
			MPI_Recv((void *)&data, 1, MPI_INT, status.MPI_SOURCE, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			consumed_count++;
			MPI_Send((void *)&data, 1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD);
		}
		flag = 0;
	}
    }

    MPI_Finalize();
    return 0;
}
