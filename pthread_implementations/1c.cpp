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

pthread_mutex_t consumer_mutex;
pthread_mutex_t producer_mutex;

pthread_cond_t producer_cond;
pthread_cond_t consumer_cond;

void* producer(void *s);
void* consumer(void *s);

//Queue for insertion at one end and extraction at the other.
struct queue 
{
	uint32_t *arr;
	int head;
	int tail;
};

struct queue *q = NULL;

//Queue operations is_full, empty, insert and remove.
bool is_full()
{
	return ((q->tail + 1)%buffer_size == q->head);
}

bool is_empty()
{
	return (q->head == q->tail);
}

void insert(int item)
{
	if (is_full()) return;

	q->arr[q->tail] = item;

	q->tail =  (q->tail+1)%buffer_size;
}

int remove()
{
	if (is_empty()) {
		return -1;
	}

	int item = q->arr[q->head];

	q->head = (q->head+1)%buffer_size;

	return item;
}

int num_of_threads = 0;
int main(int argc, char **argv)
{
        //The code has 3 command line arguments.
	//1. No of threads(k) 2. Buffer size 3. Sleep time(secs)
	
	if (argc != 4) {
		cout <<"\nUsage: ./a.out k buffer_size sleep_time\n";
		exit(1);
	}

	k = atoi(argv[1]);

	buffer_size = atoi(argv[2]);

	sleep_time = atoi(argv[3]);

	//Pthread attrtibute, mutex and condition variable initialization.
	pthread_t producers[k];
	pthread_t consumers[k];
	pthread_attr_t attr;
	pthread_mutexattr_t lockattr;

	pthread_attr_init(&attr);
	pthread_mutexattr_init(&lockattr);

	pthread_mutex_init(&producer_mutex, &lockattr);
	pthread_mutex_init(&consumer_mutex, &lockattr);

	pthread_cond_init(&producer_cond, NULL);

	pthread_cond_init(&consumer_cond, NULL);

	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	//Initialize the circular buffer structure.
	q = (struct queue*)malloc(sizeof(struct queue));
	q->arr = (uint32_t*)malloc(sizeof(uint32_t)*buffer_size);
	q->head = 0; q->tail = 0;

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

	//Sleep for a certain amount of time.
	sleep(sleep_time);

	//After sleep time is expired, kill all producer and consumer threads.
	for (i = 0; i < k; i++) {
		pthread_kill(producers[i], 0);
		pthread_kill(consumers[i], 0);
	}

	gettimeofday(&end_time, NULL);

	//Compute runtime
	double run_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - (start_time.tv_sec*1000000 + start_time.tv_usec);
	double run_time_secs = run_time_usecs/1000000;

	//Compute throughput
	double insertion_tput = num_insertions/run_time_secs;
	double extraction_tput = num_extractions/run_time_secs;

	//Compute total throughput
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
                //Acquire a mutex lock hoping that a producer write will be successful
		pthread_mutex_lock(&producer_mutex);

                //If the buffer is full, the producer gets into a condition wait
		while (is_full()) {
			pthread_cond_wait(&producer_cond, &producer_mutex);
		}

		 //Insert the produced value onto the buffer and update the no of insertions.
		insert(produced_value);
		produced_value = produced_value + k;
		num_insertions++;

		//Signal the consumer and give up the mutex.
		pthread_cond_signal(&consumer_cond);
		pthread_mutex_unlock(&producer_mutex);
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
		continue;
	}

	while (1) {
		//Acquire a mutex lock in the hope the consumer will be able to extract an item.
		pthread_mutex_lock(&consumer_mutex);

		//If the buffer is empty, consumer enters into condition wait.
		while (is_empty()) {
			pthread_cond_wait(&consumer_cond, &consumer_mutex);
		}

		//Consume a value and update the no of extracted values.
		consumed_value = remove();
		
		num_extractions++;

		//Signal the producer and give up the mutex.
		pthread_cond_signal(&producer_cond);
		pthread_mutex_unlock(&consumer_mutex);
	}
	return s;
}
