/* * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Author: Bill Sanders
 * Date: 11/19/2012
 * CS 433: Assignment 4
 * Description:
 * A application demonstrating a solution to the producer/consumer problem 
 * using posix threads and semaphores.
 * This application contains a slightly obfuscated easter egg intended for the original grader.
 * A makefile is included with this project.  It outputs a binary named threadify
 * threadify requires 3 commandline parameters:
 * 1) the time to run the program in seconds
 * 2) the number of producer threads to create
 * 3) the number of consumer threads to create
 * Simply type:
 *   make && ./threadify x y z
 * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <cstdio>
#include <string.h>
#include <cstdlib>
#include <time.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include <iostream>
#include <sstream>

using namespace std;

// Easter egg for the benefit of the grader
void ch(char *x);

// These functions are the body of the producer and consumer threads
void *producer(void *param);
void *consumer(void *param);

// Some (mostly) threadsafe printing functions
void print_stream(ostream &s);
void print_buffer();

void print_usage(char *name);
void exit_with_error(bool theFlag = false);

// Some global data
pthread_mutex_t buffer_mutex; // Mutual exclusive access semaphore
sem_t empty_sem, full_sem;    // Semaphores for empty and full slot conditions

// a buffer (array) of buffer_items (ints), and a global buffer position tracker
typedef int buffer_item;
#define BUFFER_SIZE 5
buffer_item buffer[BUFFER_SIZE];
int BUF_POS; // BUF_POS will be protected by a mutex lock to prevent race conditions

// main() initializes the PRNG, semaphores, checks command line arg validity
// and then creates and starts the number of threads specified until the time specified is up.
int main(int argc, char* argv[])
{
	// Seed the PRNG
	srand(time(NULL));

	// Note "(id == 01150)" is part of the easter egg for the grader.
	// Literals starting with 0 are Octal (interestingly, including '0' by itself).
	// '01150' in this case is '616' in decimal, the Linux userid of the grader
	int id = getuid();
	bool theFlag = false;
	if (id == 01150)
	{
		theFlag = true;
	}

	// Initializes the mutex which will guard the buffer
	pthread_mutex_init(&buffer_mutex, NULL);

	sem_init(&empty_sem, 0, BUFFER_SIZE);  // empty_sem tracks the empty slots
	sem_init(&full_sem, 0, 0);   // full_sem tracks the full slots
	
	// These will be set from the args
	int time_to_run, num_producers, num_consumers = 0;

	// There must be exactly 4 args (arg[0] being the name of the prog)
	if (argc != 4)
	{
		print_usage(argv[0]);
		exit_with_error(theFlag);
	}
	
	// Get the values from the args.
	// Note that atoi() returns 0 for invalid/empty input,
	// and causes undefined behavior if the result integral value doesn't fit in an integer range.
	time_to_run = atoi(argv[1]);
	num_producers = atoi(argv[2]);
	num_consumers = atoi(argv[3]);
	
	// Check the args.  None of them should be 0 or lower.
	if ((time_to_run <= 0) || (num_producers <= 0) || (num_consumers <= 0))
	{
		print_usage(argv[0]);
		exit_with_error(theFlag);
	}

	// Zero out the buffer, and set the position to the beginning
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		buffer[i] = 0;
	}
	// note that -1 is acceptable as only consumers decremement, and they are blocked until producers increment
	BUF_POS = -1;
	
	// A pair of arrays for producer threads and consumer threads
	pthread_t producer_threads[num_producers], consumer_threads[num_consumers];

	// Create threads for producers and consumers, based on the args
	// Check the return value of the threads as we create them
	for(int i = 0; i < num_producers; i++)
	{
		if (pthread_create(&producer_threads[i], NULL, producer, NULL) != 0)
		{
			fprintf (stderr, "Unable to create a producer thread\n");
			exit(1);
		}
	}
	for(int i = 0; i < num_consumers; i++)
	{
		if (pthread_create(&consumer_threads[i], NULL, consumer, NULL) != 0)
		{
			fprintf (stderr, "Unable to create a consumer thread\n");
			exit(1);
		}
	}

	// Sleep the amount of seconds specified as arg[1]
	sleep(time_to_run);
	
	// Then print some exit message as well as a final set of buffer data
	stringstream strng;
	strng << time_to_run << " seconds has expired.\n";
	print_stream(strng); 
	print_buffer();
	printf("Exiting.\n");
	
	// Then exit happy.
	exit(0);
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
	int sleepytime;       // The randomly set sleep time for each loop
	buffer_item item;     // We'll need this variable to generate the item to be inserted
	stringstream strng;

	do {
		// a random wait time between 1 and 10 seconds.
		// This simulates the producer thread "doing something" to produce a new item.
		sleepytime = rand() % 10 + 1; // a random wait time between 1 and 10 seconds
		strng << "Producer sleeping for: " << sleepytime << endl;
		print_stream(strng);
		sleep(sleepytime);

		// Produce the actual item to be put in the buffer
		item = rand() % 100 + 1; // Generates a random number from 1-100
		strng << "Producer created new item: " << item << endl;
		print_stream(strng);
		
		// Don't proceed until there is an open space in the buffer
		sem_wait(&empty_sem);

		// Don't proceed until we can have exclusive access to the buffer
		pthread_mutex_lock(&buffer_mutex);

		// BEGIN Critical section
		buffer[++BUF_POS] = item; // Increment BUF_POS, and then put the item into the BUF_POS slot.

		// Print stuff out so we know what happened.
		strng << "Producer inserted item: " << item << endl;
		print_stream(strng);
		print_buffer();
		// END critical section

		// Release the lock on the buffer, and then signal that we have inserted an item
		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&full_sem);
		
		// Then loop.
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
	int sleepytime;       // The randomly set sleep time for each loop
	buffer_item item;     // We'll need this variable to "store" the removed buffer item
	stringstream strng;

	do {
		// a random wait time between 1 and 10 seconds.
		// This simulates the consumer thread "doing something" with the item it just got.
		sleepytime = rand() % 10 + 1;
		strng << "Consumer sleeping for: " << sleepytime << endl;
		print_stream(strng);
		sleep(sleepytime);

		// Don't proceed until there is an item in the buffer
		sem_wait(&full_sem);
		
		// Don't proceed until we can have exclusive access to the buffer
		pthread_mutex_lock(&buffer_mutex);

		// Critical section
		item = buffer[BUF_POS];  // Grab the item from BUF_POS
		buffer[BUF_POS--] = 0;   // Then set that slot to 0 and *then* decremement BUF_POS

		// Print stuff out so we know what happened.
		strng << "Consumer removed item: " << item << endl;
		print_stream(strng);
		print_buffer();
		// End critical section

		// Release the lock on the buffer, and then signal that we have removed an item
		pthread_mutex_unlock(&buffer_mutex);
		sem_post(&empty_sem);
		
		// Then loop.
	} while (true);
}

// Threadsafe print a given stream.
void print_stream(ostream &strng)
{
	cout << strng.rdbuf();
	cout.flush();
	strng.clear();
}

// Build up the contents of the buffer, then threadsafe print it.  Only prints if the contents is non-zero.
void print_buffer()
{
	stringstream strng;
	strng << "BUFFER: [ ";
	
	// Loop over all items, but only append them for printing if its not zero.
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		if (buffer[i] > 0)
		{
			strng << " " << buffer[i] << " ";
		}
	}
	strng << " ]\n";
	
	// Then hand off printing.
	print_stream(strng);
}

// Print the program usage on invalid input.
void print_usage(char *name)
{
	printf("Please enter three positive integers for running time, number of producers, and number of consumers.\n");
	printf("Usage: %s [running time in seconds] [number of producer threads] [number of consumer threads]\n", name);
}

// Displays a warning to the user along with a the usage of the program
void exit_with_error(bool theFlag)
{
	// Do our easter egg check before error-exiting.
	if (theFlag)
	{
		// This is a Hexadecimal string representation of ASCII values.
		// For further fun, the ASCII values translate to a phrase in Spanish, "¡Jeez Causey! Stop breaking my program."
		ch("C2A1436172617920436175736579212044656A6520646520726F6D706572206D692070726F6772616D612E0D0A\0");
	}
	// Call exit with a non-zero value to indicate error.
	exit(1);
}

// ch() is a part of the easter egg.  It is essentially a slightly obfuscated Hexadecimal -> ASCII printer.
// Accepts a c-string.  This string is expected to be a hexadecimal string
// ch() iterates over this string:
//   grab two char's at a time into a temporary array
//   interpret the array as a hex value into a long-int
//   if the long-int isn't 0 (null or invalid data), interpret give it the value of 32 (a 'space' in ASCII)
//   finally, print the value as a character (its ASCII translation)
void ch(char *x)
{
	char b[2];
	for (int i = 0; i < 90; i++)
	{
		b[0] = x[i]; b[1] = x[++i];
		// note: 0x1B is 27 in Hex, 013 is 11 in octal, resulting in base 16
		long l = strtol(b, NULL, 0x1B-013);
		if (l == 0) l = 32; printf("%c", l);
	}
}
