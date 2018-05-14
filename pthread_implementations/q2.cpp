#include <iostream>
#include <stdint.h>
#include <iomanip>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <queue>

using namespace std;

//Binary search tree structure.
struct binary_search_tree
{
	int data;
	struct binary_search_tree *left;
	struct binary_search_tree *right;
	pthread_rwlock_t tree_lock;
};

struct binary_search_tree *tree_root = NULL;

int k = 0, num_of_threads = 0;

int *arr = NULL;

int *values_per_thread = NULL;
int *starting_index = NULL;
int total_num_values = 10;
int root_thread = 0;
void* insertions(void *s);
void* lookups(void *s);
void inorder_traversal(struct binary_search_tree *);
void level_order_traversal(struct binary_search_tree *);


//Optional helper routine to print values which will be inserted into the BST.
void print_value_info()
{
	int i;
	cout << "Array contents:\n";
	for (i = 0; i < total_num_values; i++) {
		cout << arr[i] << " ";
	}

	cout << "\n";

	cout << "Starting index\n";
	for (i = 0; i < k; i++) {
		cout << starting_index[i] << " ";
	}

	cout << "\n";

	cout << "Values per thread:\n";
	for (i = 0; i <k; i++) {
		cout << values_per_thread[i] << " ";
	}

	cout << "\n";
}

int main(int argc, char **argv)
{
	//This code takes a single argument, the no of concurrent threads to use.
	if (argc != 2) {
		cout << "\nUsage ./a.out <num_of_threads>\n";
		exit(1);
	}

	k = atoi(argv[1]);

	//Initialize pthread attributes.
	pthread_t pthreads[k];
	pthread_t pthread_lookups[k];

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	struct timeval start_time, end_time;

	int i = 0, j = 0;

	//We split the no of values to be inseted (256 in this case) amost equally among all threads. ex: with 8 threads, each thread shall insert 32 values.
	int nums_per_thread_approx = total_num_values/k;

	arr = new int[total_num_values];

	values_per_thread = new int[k];
	starting_index = new int[k];

	//The root of the tree shall be the middle value of the sequence of no's (128 in our case).
	int root_value = (int)(1+total_num_values)/2;
	int root_index = -1;

	int value_to_begin = 1;

	//Code to shuffle values among various threads.
	//This is to ensure that we do not have a skewed tree with just 1 value at each level.
	//Rather than having threads generate values p, p+k, p+2k,... dynamically, we precompute them
	//and store the results in an array.
	while (i < total_num_values) {
		int num_values = 0;
		starting_index[value_to_begin-1] = i;
		for (j = value_to_begin; j <= total_num_values; j=j+k) {
			if (j == root_value) {
				root_thread = value_to_begin;
				root_index = i;
			}
			arr[i++] = j;
			num_values++;
		}
		values_per_thread[value_to_begin-1] = num_values;
		value_to_begin++;
	}

	//Compute the root value to be inserted during BST construction. We ensure that the root is the middle
	//element of the series 1,2,3,..256 i.e approximately 128. This gives us a more balanced BST.
	int temp = arr[root_index];
	int start_index = starting_index[root_thread-1];

	arr[root_index] = arr[start_index];
	arr[start_index] = temp;

	//Optional routine call to check what values are stored in the precomputed array.
//	print_value_info();

	//Create threads to construct the BST by performing parallel insertions.
	int x[k];
	for (i = 0; i < k; i++) {
		x[i] = i+1;
		pthread_create(&pthreads[i], &attr, insertions,
				(void*)&x[i]);
		num_of_threads++;
	}

	gettimeofday(&start_time, NULL);

	for (i = 0; i < k; i++) {
		pthread_join(pthreads[i], NULL);
	}

	gettimeofday(&end_time, NULL);

	//Optional routine to traverse the constructed tree in order and level order.
/*	cout<<"\nInorder BST tree traversal\n";
	inorder_traversal(tree_root);
	cout<<"\n\nLevel Order BST tree traversal\n";
	level_order_traversal(tree_root);*/


	//Compute the total insertion time.
	double run_time_usecs = (end_time.tv_sec*1000000 + end_time.tv_usec) - 
				(start_time.tv_sec*1000000 + start_time.tv_usec);
	
	double run_time_msecs = run_time_usecs/1000;

	//Compute insertion throughput.
	double insert_tput = (double)total_num_values*1000000/run_time_usecs;
	
	cout << "Insertion runtime: " << run_time_msecs << " millisecs\n";
	cout << "Insertion throughput: " << insert_tput << " ops/second\n";

	struct timeval start_time2, end_time2;

	//Now create threads to perform parallel lookups.
	num_of_threads = 0;
	for (i = 0; i < k; i++) {
		pthread_create(&pthread_lookups[i], &attr, lookups,
				(void*)&x[i]);
		num_of_threads++;
	}

	gettimeofday(&start_time2, NULL);
	for (i = 0; i < k; i++) {
		pthread_join(pthread_lookups[i], NULL);
	}
	gettimeofday(&end_time2, NULL);

	//Compute BST lookup runtime.
	run_time_usecs = (end_time2.tv_sec*1000000 + end_time2.tv_usec) -
			 (start_time2.tv_sec*1000000 + start_time2.tv_usec);
	run_time_msecs = run_time_usecs/1000;

	//Compute BST lookup throughput.
	double lookup_tput = (double)total_num_values*1000000/run_time_usecs;

	cout << "Lookup runtime: " << run_time_msecs << " millisecs\n";
	cout << "Lookup throughput: " << lookup_tput << " ops/second\n";
	
	return 0;	
}


//Routine to perform BST insertions.
void* insertions(void* s)
{
	int *value = (int*)s;
	int p = *value;

	if (p == 0) p++;

	int num_values_to_insert = values_per_thread[p-1];
	int index = starting_index[p-1];
	pthread_rwlockattr_t lockattr;
	pthread_rwlockattr_init(&lockattr);
	int inserted_values = 0;

	//Wait for all threads to be created. This functions like an implicit barrier.
	//We do not need to put the threads to sleep using condition wait barrier since
	//throughput will anyways be computed from the point we start computations.
	while (num_of_threads< k) {
	}

	while (1) {
		//When a given thread is done inserting the specified no of values, exit the thread.
		if (inserted_values == num_values_to_insert) {
			pthread_exit(0);
		}

		if (tree_root == NULL) {
			//Other threads shall wait until the root node is inserted.
			if ( p != root_thread) {
				continue;
			} else {
				//Insert the root node.
				struct binary_search_tree *node = (struct binary_search_tree*)
					malloc(sizeof(struct binary_search_tree));
				node->left = node->right = NULL;
				node->data = arr[index];
				pthread_rwlock_init(&node->tree_lock, &lockattr);
				tree_root = node;
				index++;
				inserted_values++;
				continue;
			}
		}

		int data = arr[index];
		struct binary_search_tree *node = tree_root;
		pthread_rwlock_rdlock(&node->tree_lock);
		while (node != NULL) {
			if (data < node->data) {
				if (node->left == NULL) {
					//Give up the read lock and attempt to acquire the write lock for insertion.
					pthread_rwlock_unlock(&node->tree_lock);
					pthread_rwlock_wrlock(&node->tree_lock);

					//Potential race where another thread inserts at the same location.
					//Find a new location to insert.
					if (node->left != NULL) {
						pthread_rwlock_unlock(&node->tree_lock);
						pthread_rwlock_rdlock(&node->tree_lock);
						continue;
					}
					//BST left node is free. Insert here.
					struct binary_search_tree *newnode = (struct binary_search_tree*)
						malloc(sizeof(struct binary_search_tree));
					newnode->data = data;
					newnode->left = newnode->right = NULL;
					pthread_rwlock_init(&newnode->tree_lock, &lockattr);
					node->left = newnode;
					pthread_rwlock_unlock(&node->tree_lock);

					index++;
					inserted_values++;
					break;
				} else {
					//Hand over hand locking:Get a lock on successor before unlocking predecessor.
					//Navigate to the left subtree.
					pthread_rwlock_rdlock(&node->left->tree_lock);
					pthread_rwlock_unlock(&node->tree_lock);
					node = node->left;
				}
			} else {
				if (node->right == NULL) {
					//Give up the read lock and attempt to acquire the write lock for insertion.
					pthread_rwlock_unlock(&node->tree_lock);
					pthread_rwlock_wrlock(&node->tree_lock);

					//Potential race where another thread inserts at the same location.
					//Find a new location to insert.
					if (node->right != NULL) {
						pthread_rwlock_unlock(&node->tree_lock);
						pthread_rwlock_rdlock(&node->tree_lock);
						continue;
					}
					//BST right node is free. Insert here.
					struct binary_search_tree *newnode = (struct binary_search_tree*)
						malloc(sizeof(struct binary_search_tree));
					newnode->data = data;
					newnode->left = newnode->right = NULL;
					pthread_rwlock_init(&newnode->tree_lock, &lockattr);
					node->right = newnode;
					pthread_rwlock_unlock(&node->tree_lock);

					index++;
					inserted_values++;
					
					break;
				} else {
					//Hand over hand locking:Get a lock on successor before unlocking predecessor.
					//Navigate to the right subtree.
					pthread_rwlock_rdlock(&node->right->tree_lock);
					pthread_rwlock_unlock(&node->tree_lock);
					node = node->right;
				}
			}
			
		}
	}

	return s;
}


void* lookups(void *s)
{
	int *value = (int *)s;

	int p = *value;
	if (p == 0) p++;

	int index = starting_index[p-1];
	int num_values_to_lookup = values_per_thread[p-1];
	int curr_lookup_count  = 0;

	//Wait for all threads to be created. This functions like an implicit barrier.
	//We do not need to put the threads to sleep using condition wait barrier since
	//throughput will anyways be computed from the point we start computations.
	while (num_of_threads < k) {
	}

	while (1) {
		//If given thread is done looking up all the values assigned to it, exit the thread.
		if (curr_lookup_count == num_values_to_lookup) {
			pthread_exit(0);
		}

		struct binary_search_tree *node = tree_root;
		int value_to_lookup = arr[index];
		
		pthread_rwlock_rdlock(&node->tree_lock);
	
		while (node != NULL) {
			//Found value. Now increment the lookup count and proceed to find the next value.
			if (value_to_lookup == node->data) {
				curr_lookup_count++;
				index++;
				break;
			} else if (value_to_lookup < node->data) {
				//The requested value could not be found. Report this to the user.
				if (node->left == NULL) {
					cout << "Value " << value_to_lookup << " not found\n";
					pthread_rwlock_unlock(&node->tree_lock);
					curr_lookup_count++;
					index++;
					break;
				}
                                //Hand over hand locking:Get a lock on successor before unlocking predecessor.
				//Navigate to the right subtree.
				pthread_rwlock_rdlock(&node->left->tree_lock);
				pthread_rwlock_unlock(&node->tree_lock);
				node = node->left;	
			} else {
		                //The requested value could not be found. Report this to the user.
				if (node->right == NULL) {
					cout << "Value " << value_to_lookup << " not found\n";
					pthread_rwlock_unlock(&node->tree_lock);
					curr_lookup_count++;
					index++;
					break;
				}

			        //Hand over hand locking:Get a lock on successor before unlocking predecessor.
		                //Navigate to the right subtree.
				pthread_rwlock_rdlock(&node->right->tree_lock);
				pthread_rwlock_unlock(&node->tree_lock);
				node = node->right;
			}
		}
	}
	return s;
}

//Optional routine to perform a level order traversal of a BST.
void level_order_traversal(struct binary_search_tree *root)
{
	if (root == NULL) return;

	struct binary_search_tree *temp = root;

	queue<struct binary_search_tree*> q;
	q.push(temp);
	q.push(NULL);

	int level = 1;
	cout << "Level " << level << ": ";
	while(!q.empty()) {
		temp = q.front();
		q.pop();

		if (temp == NULL) {
			cout << "\n";
			if (!q.empty()) {
				q.push(NULL);
				level++;
				cout << "Level " << level << ": ";
			}
			continue;
		}

		cout << temp->data << " ";
		
		if (temp->left) q.push(temp->left);
		if (temp->right) q.push(temp->right);
	}
	cout << "\n";
}


//Optional routine to perform inorder traversal of a BST.
void inorder_traversal(struct binary_search_tree *root)
{
	if (root == NULL) return;

	inorder_traversal(root->left);
	cout << root->data << " ";
	inorder_traversal(root->right);
}
