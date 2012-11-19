#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include <iostream>
#include <sstream>

#include "buffer.h"

using namespace std;

// void *print_message_function( void *ptr );
void *producer(void *param);
void *consumer(void *param);
void print_stream(ostream &s);
void print_buffer();

pthread_mutex_t buffer_mutex;
sem_t empty_sem, full_sem;
int BUF_POS = -1;
buffer_item buffer[BUFFER_SIZE];


int main(int argc, char* argv[])
{
	srand(time(NULL));
	
	pthread_mutex_init(&buffer_mutex, NULL);

	sem_init(&empty_sem, 0, BUFFER_SIZE);  // The number of empty slots
	sem_init(&full_sem, 0, 0);   // the number of full slots
	
	int num_producers, num_consumers, time_to_sleep = 0;
	if (argc != 4)
	{
		printf("Dude, don't break the program.\n");
		exit(1);
	}
	
	time_to_sleep = atoi(argv[1]);
	num_producers = atoi(argv[2]);
	num_consumers = atoi(argv[3]);
	
	// Causey check the args
	if ((time_to_sleep <= 0) || (num_consumers <= 0) || (num_producers <= 0))
	{
		printf("Stop trying to break my program.\n");
		exit(1);
	}

	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		buffer[i] = 0;
	}

	
	pthread_t producer_threads[num_producers], consumer_threads[num_consumers];
	//int  iret1, iret2;

	// Create threads for producers and consumers
	for(int i = 0; i < num_producers; i++)
	{
		pthread_create(&producer_threads[i], NULL, producer, NULL);
	}
	for(int i = 0; i < num_consumers; i++)
	{
		pthread_create(&consumer_threads[i], NULL, consumer, NULL);
	}
	
// 	do {
// 		pthread_mutex_lock(&buffer_mutex);
// 		printf("BUFFER: [ ");
// 		for (int i = 0; i < BUFFER_SIZE; i++)
// 		{
// 			printf("%d, ", buffer[i]);
// 		}
// 		printf(" ]\n");
// 		pthread_mutex_unlock(&buffer_mutex);
// 		time_to_sleep = time_to_sleep - 3;
// 		sleep(3);
// 	} while (time_to_sleep >= 0);
// 	exit(0);

	sleep(time_to_sleep);
	
	print_stream(stringstream() << time_to_sleep << " seconds has expired.\n");
	print_buffer();
	print_stream(stringstream() << "Exiting.\n");
	
	exit(0);

// 	iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
// 	iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);

	/* Wait till threads are complete before main continues. Unless we  */
	/* wait we run the risk of executing an exit which will terminate   */
	/* the process and all threads before the threads have completed.   */

// 	for(int i = 0; i < num_producers; i++)
// 	{
// 		pthread_join(producer_threads[i], NULL);
// 	}
// 	for(int i = 0; i < num_consumers; i++)
// 	{
// 		pthread_join(consumer_threads[i], NULL);
// 	}

// 	pthread_join( thread1, NULL);
// 	pthread_join( thread2, NULL); 
}


/* 
loop:
	Produce the item
	wait until the buffer has an empty slot
	wait for exclusive access to the buffer
	critical section:
		add produced item to buffer
	signal mutex that we are done with buffer
	increase the buffer full count

*/
void *producer(void *param)
{
	// Produce the actual item to be put in the buffer
	do {
		int sleepytime = rand() % 10 + 1;
		print_stream(stringstream() << "Producer " << (unsigned long) pthread_self() << " sleeping for: " << sleepytime << "\n");
		sleep(sleepytime);

		buffer_item item = rand() % 100;
		print_stream(stringstream() << "Producer " << pthread_self() << " created new item: " << item << "\n");
//		printf("Producer %lX created new item: %d\n", (unsigned long) pthread_self(), item);
		
		sem_wait(&empty_sem);
		pthread_mutex_lock(&buffer_mutex);
		print_stream(stringstream() << "Producer " << pthread_self() << " inserted item: " << item << "\n");
		print_buffer();
//		printf("item: %d @ %d\n", item, BUF_POS);
//		buffer[BUF_POS + 1] = item;
// 		BUF_POS++;
		buffer[++BUF_POS] = item;
		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&full_sem);
		
//		print_stream(stringstream() << "Producer " << pthread_self() << " sleeping: " << sleepytime << "\n");
//		printf("Producer %lX exiting!\n", (unsigned long) pthread_self());
	} while (true);
	
}


/* 
loop:
	wait until the buffer has an item
	wait for exclusive access to the buffer
	critical section:
		remove an item from buffer
	signal mutex that we are done with buffer
	increase the buffer empty count
	consume the item

*/
void *consumer(void *param)
{
	// Produce the actual item to be put in the buffer
//	rand() % RAND_MAX;
	do {
		int sleepytime = rand() % 10 + 1;
		print_stream(stringstream() << "Consumer " << pthread_self() << " sleeping for: " << sleepytime << "\n");
		sleep(sleepytime);
		
		int item = 0;
	//	char *message2 = "I'm a consumer!";
		sem_wait(&full_sem);
		pthread_mutex_lock(&buffer_mutex);
		item = buffer[BUF_POS];
		print_stream(stringstream() << "Consumer " << pthread_self() << " removed item: " << item << "\n");
		print_buffer();
//		printf("got item: %d @ POS %d\n", item, BUF_POS);
// 		buffer[BUF_POS] = 0;
// 		BUF_POS--;
		buffer[BUF_POS--] = 0;
//		printf("POS now %d\n", BUF_POS);
		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&empty_sem);

//		printf("Consumer %lX got item: %d\n", (unsigned long) pthread_self(), item);
		
//		printf("Consumer %lX exiting!\n", (unsigned long) pthread_self());
	} while (true);
	
}

// void *print_message_function( void *ptr )
// {
// 	char *message;
// 	message = (char *) ptr;
// 	printf("%s \n", message);
// }

void print_stream(ostream &s)
{
	cout << s.rdbuf();
	cout.flush();
	s.clear();
}

void print_buffer()
{
	stringstream strng;
	strng << "BUFFER: [ ";
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		if (i > 0)
		{
			strng << " " << buffer[i] << " ";
		}
	}
	strng << " ]\n";
	print_stream(strng);	
}