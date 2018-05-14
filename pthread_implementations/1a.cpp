#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

using namespace std;

int num_insertions = 0, num_extractions = 0;

int buffer_size = 0;
int k = 0;
int sleep_time = 0;

pthread_rwlock_t read_write_lock;

void* producer(void *s);
void* consumer(void *s);

int num_of_threads = 0;

//Stack for insertion and extraction from end of a buffer.
struct stack {
	uint32_t *arr;
	int top;
};

struct stack *buffer = NULL;

//Stack operations is_full, empty, push and pop.
bool is_full()
{
	return buffer->top == buffer_size -1;
}

bool empty() {
	return buffer->top == -1;
}

void push(uint32_t item)
{
	if (is_full()) return;
        *(buffer->arr + ++(buffer->top)) = item;
}

uint32_t pop()
{
	if (empty()) return 0;
        uint32_t item = *(buffer->arr + buffer->top--);

	return item;
}

int main(int argc, char **argv)
{
	//The code has 3 command line arguments. 
	//1. No of threads(k) 2. Buffer size 3. Sleep time(secs)
	if (argc != 4) {
		cout <<"\nUsage: ./a.out k buffer_size sleep_time(in seconds)\n";
		exit(1);
	}

	k = atoi(argv[1]);

	buffer_size = atoi(argv[2]);

	sleep_time = atoi(argv[3]);

	buffer = (struct stack *)malloc(sizeof(struct stack));

	buffer->arr = (uint32_t*)malloc(sizeof(uint32_t)*buffer_size);
	buffer->top = -1;

	//Pthread attribute and rw lock initialization.
	pthread_t producers[k];
	pthread_t consumers[k];
	pthread_attr_t attr;
	pthread_rwlockattr_t lockattr;

	pthread_attr_init(&attr);
	
	pthread_rwlock_init(&read_write_lock, NULL);

	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	struct timeval start_time, end_time;
	int i = 0;

	int x[k];
	
	//Create k producer and consumer threads.
	for (i = 0; i < k; i++) {
		x[i] = i+1;
		pthread_create(&producers[i], &attr, producer, 
				(void *)&x[i]);
		pthread_create(&consumers[i], &attr, consumer, (void*)&x[i]);
		num_of_threads +=  2;
	}

	gettimeofday(&start_time, NULL);

	// Sleep for a certain amount of tme.
	sleep(sleep_time);

	//After sleep time is expired, kill all producer and consumer threads.
	for (i = 0; i < k; i++) {
		pthread_kill(producers[i], 0);
		pthread_kill(consumers[i], 0);
	}

	gettimeofday(&end_time, NULL);

	//compute run time.
	double run_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - (start_time.tv_sec*1000000 + start_time.tv_usec);
	double run_time_secs = run_time_usecs/1000000;

	//Compute throughput.
	double insertion_tput = (double)num_insertions/run_time_secs;
	double extraction_tput = (double)num_extractions/run_time_secs;

	//Compute total throughput.
	double total_tput = (num_insertions+num_extractions)/run_time_secs;

	cout << fixed;
	cout << "\nNumber of insertions: " << num_insertions << " Number of extractions: " << num_extractions << "\n";
	cout << "Insertion throughput = " << insertion_tput << " ops/second\n";
	cout << "Extraction throughput = " << extraction_tput << " ops/second\n";
	cout << "Total throughput = " << total_tput << " ops/second\n";

	return(0);
}

void* producer(void *s)
{
	int *arg = (int*)s;

	int p = *arg;
	if (p == 0) p++;

	int produced_value = p;
	int total = 2*k;

	//Wait for all threads to be created. This functions like an implicit barrier.
	//We do not need to put the threads to sleep using condition wait barrier since
	//throuhput will anyways be computed from the point we start computations.
	while (num_of_threads < total) {
	}

	while (1) {
		
		//If buffer is full, producer cannot proceed.
		//We don't need a read lock to check whether buffer is full since we anyways
		//perform that check before writing. An unwanted read lock gives very poor throughput.
		if (is_full()) {
		 	continue;	
		}

		//Acquire a write lock before the producer writes into the buffer.
		pthread_rwlock_wrlock(&read_write_lock);
		//If the buffer is full, abort the write and get back to spinning stage.
		if (is_full()) {
			pthread_rwlock_unlock(&read_write_lock);
			continue;
		}

		//cout <<"Producer thread " << p << " Produced value " << produced_value << "\n";
		
		//Push the produced value onto the buffer and update the no of insertions.
		push(produced_value);
		produced_value = produced_value + k*p;
		num_insertions++;
		pthread_rwlock_unlock(&read_write_lock);
	}

	return s;
}

void* consumer(void *s)
{
	int *init_value = (int*)s;
	int p = *init_value;

	if (p == 0) p++;

	int consumed_value = 0;

	int total = 2*k;

	//Wait for all threads to be created. This functions like an implicit barrier.
	//We do not need to put the threads to sleep using condition wait barrier since
	//throughput will anyways be computed from the point we start computations.

	while (num_of_threads < total) {
	}

	while (1) {
	
		//If buffer is empty, consumer cannot proceed.
		//We don't need a read lock to check whether buffer is empty since we anyways
		//perform that check before extracting. An unwanted read lock gives very poor throughput.
		if (empty()) {
			continue;
		}


		//Acquire a write lock before extracting data from the buffer.
		pthread_rwlock_wrlock(&read_write_lock);
		//Abort extraction if buffer is empty. Resume spin check in a loop.
		if (empty()) {
			pthread_rwlock_unlock(&read_write_lock);
			continue;
		}
	
		//Extract a value from the buffer and update the no of extractions.
		consumed_value = pop();

		num_extractions++;
		pthread_rwlock_unlock(&read_write_lock);
	}

	return s;
}
