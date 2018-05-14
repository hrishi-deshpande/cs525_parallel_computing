#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <limits.h>

#define PROD_TAG 1
#define CONS_TAG 2
#define ACK_TAG 3
#define SLEEP_TIME 120

int buffer_size = 2, my_rank = 0, world_size = 0;
uint64_t consumed_count = 0;
MPI_Comm MPI_COMM_CONS;

//Queue for insertion at one end and extraction at the other.
struct queue 
{
	int *arr;
	int head;
	int tail;
	int size;
};

struct queue *q = NULL;

//Queue operations is_full, empty, insert and remove.
int is_full()
{
	return (q->size == buffer_size);
}

int is_empty()
{
	return (q->size == 0);
}

void insert(int item)
{
	if (is_full()) return;

	q->arr[q->tail] = item;

	q->tail =  (q->tail+1)%buffer_size;

	q->size++;
}

int delete()
{
	if (is_empty()) {
		return -1;
	}

	int item = q->arr[q->head];

	q->head = (q->head+1)%buffer_size;

	q->size--;

	return item;
}

void my_terminate_handler() {
	int data = INT_MIN, i = 0;
	int flag = 0;
	MPI_Request request;
	MPI_Status status;

	uint64_t final_count = 0;

	struct timeval start_time, end_time;

	double time = 5, time_diff = 0;

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

	for (i = 1; i < world_size; i++) {
		data = INT_MIN;
		if (i %2 == 0) {
			MPI_Send((void*)&data, 1, MPI_INT, i, CONS_TAG, MPI_COMM_WORLD);
		} else {
			MPI_Send((void*)&data, 1, MPI_INT, i, ACK_TAG, MPI_COMM_WORLD);
		}
	}

	MPI_Reduce((void *)&consumed_count, (void *)&final_count, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_CONS); 
	printf("\nTotal consumed count = %lu\n", final_count);

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
    int flag;
    int data = 0;

    MPI_Status status;
    MPI_Request request;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int color = world_size / 2;

    if (world_size < 3) {
	printf("\nNo of processes should be atleast 3 for the system to work\n");
	MPI_Finalize();
	exit(1);
    }

    if (world_size %2 == 0) {
	buffer_size = world_size;
    } else {
	buffer_size = (world_size/2)*2;
    }

    if (my_rank == 0) {	
	MPI_Comm_split(MPI_COMM_WORLD, color, my_rank, &MPI_COMM_CONS);

	//Processor with rank 0 is the broker.
	q = (struct queue *)malloc(sizeof(struct queue));

	q->arr = (int *)malloc(buffer_size*sizeof(int));

	q->head = q->tail = q->size = 0;
	bzero((void *)q->arr, buffer_size*sizeof(int));

	struct timeval start_time;

	gettimeofday(&start_time, NULL);
		
	while (1) {
		//If the buffer is full, just receive from the consumer.
		if (is_full()) {
			MPI_Recv((void *)&data, 1, MPI_INT, MPI_ANY_SOURCE, CONS_TAG, MPI_COMM_WORLD, &status);
			if (status.MPI_SOURCE % 2 != 0) {
				printf("\nWarning. Receiving consumption message from a non-consumer\n");
				continue;
			}

			check_time(start_time);

			MPI_Send((void *)&data, 1, MPI_INT, status.MPI_SOURCE, CONS_TAG, MPI_COMM_WORLD);
		} else if (is_empty()) {
			//If the buffer is empty, just receive from the producer.
			MPI_Recv((void *)&data, 1, MPI_INT, MPI_ANY_SOURCE, PROD_TAG, MPI_COMM_WORLD, &status);

			check_time(start_time);
			insert(data);
			MPI_Send((void *)&data,1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD); 
		} else {
			//Else receive messages from any source.
			MPI_Recv((void *)&data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			check_time(start_time);

			if (status.MPI_SOURCE % 2 == 1 && status.MPI_TAG == PROD_TAG) {
				//This is a message from the producer.
				insert(data);
				MPI_Send((void *)&data, 1, MPI_INT, status.MPI_SOURCE, ACK_TAG, MPI_COMM_WORLD); 

			} else if (status.MPI_SOURCE % 2 == 0 && status.MPI_TAG == CONS_TAG) {
				//This is a message from the consumer.
				MPI_Send((void *)&data, 1, MPI_INT, status.MPI_SOURCE, CONS_TAG, MPI_COMM_WORLD);
			} else {
				//Invalid message.
				printf("\nInvalid Tag or source. Ignoring this messsage\n");
			}
		}
	}
	
    } else if (my_rank % 2 != 0) {
	//Processors with odd rank are producers.
	MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, my_rank, &MPI_COMM_CONS);

    	while (1) {
		srand(time(NULL) + my_rank);

		//Generate random data.
		data = rand();
	
		MPI_Send((void *)&data, 1, MPI_INT, 0, PROD_TAG, MPI_COMM_WORLD);

		MPI_Recv((void *)&data, 1, MPI_INT, 0, ACK_TAG, MPI_COMM_WORLD, &status);

		if (data == INT_MIN) {
			MPI_Finalize();
			exit(0);
		}
	}
	
    } else {
    	//Processors with even rank are consumers.
	MPI_Comm_split(MPI_COMM_WORLD, color, my_rank, &MPI_COMM_CONS);

	while (1) {
		MPI_Send((void *)&data, 1, MPI_INT, 0, CONS_TAG, MPI_COMM_WORLD);

		MPI_Recv((void *)&data, 1, MPI_INT, 0, CONS_TAG, MPI_COMM_WORLD, &status);

		if (data == INT_MIN) {
			int final_count = 0;
			MPI_Reduce((void *)&consumed_count, (void *)&final_count, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_CONS); 

			MPI_Finalize();
			exit(0);
		}

		consumed_count++;
	}
    }

    return 0;
}
