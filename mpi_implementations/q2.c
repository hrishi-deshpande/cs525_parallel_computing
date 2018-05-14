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

uint64_t consumed_count = 0;

int my_rank = 0, world_size = 0;

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

int main(int argc, char **argv) {
    int world_size, flag, flag2 = 0;
    int data = 0;
    int random_proc = 0;

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

    gettimeofday(&start_time, NULL);

    while (1) {
    	do {
		random_proc = rand() % world_size;
	} while (random_proc == my_rank);

	srand(time(NULL) + my_rank);

	data = rand();

	check_time(start_time);	

	MPI_Send((void*)&data, 1, MPI_INT, random_proc, MSG_TAG, MPI_COMM_WORLD);
//	MPI_Isend((void*)&data, 1, MPI_INT, random_proc, MSG_TAG, MPI_COMM_WORLD, &request);

	MPI_Iprobe(MPI_ANY_SOURCE, ACK_TAG, MPI_COMM_WORLD, &flag, &status);

	while (flag == 0) {
		MPI_Iprobe(MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &flag2, &status2);
		if (flag2 != 0) {
			MPI_Recv((void *)&data, 1, MPI_INT, status2.MPI_SOURCE, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			consumed_count++;
//			MPI_Isend((void *)&data, 1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD, &request);
			MPI_Send((void *)&data, 1, MPI_INT, status2.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD);
			flag2 = 0;
		}

		check_time(start_time);

		MPI_Iprobe(MPI_ANY_SOURCE, ACK_TAG, MPI_COMM_WORLD, &flag, &status);
	}

	MPI_Recv((void*)&data, 1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	flag = 0;
    }

    return 0;
}
