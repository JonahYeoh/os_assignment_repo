/**
 *
 * Operating Systems
 * 
 * Lab 4
 *
 **/
#include <stdlib.h>     /* NULL */
#include  <stdio.h>	/* printf */
#include  <sys/types.h>	/* pid_t */
#include <unistd.h>	/* get_pid */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <sys/wait.h> 	/* wait */
#include <pthread.h>

#define PRODUCER_NO 20	//Number of producers
#define NUM_PRODUCED 2000 //Number of items to be produced

void *generator_function(void *arg);
void *print_function(void *arg);

long sum; /* Sum of generated values*/
long finished_producers; /* number of the producer that finished producing */

pthread_t tid[PRODUCER_NO];
pthread_t print_thread;
//C: Mutex declaration and initialization
pthread_mutex_t lock;
//F: Condition variable declaration and initialization
pthread_cond_t all_done = PTHREAD_COND_INITIALIZER;

int main(void) {
    /* initialize random seed: */
    srand(time(NULL));
    sum = 0;

    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    }

    //A: Creates five generator thread
    int i, create_status, join_status;
    for ( i=0; i<PRODUCER_NO; i++ ){
    	create_status = pthread_create(&(tid[i]), NULL, &generator_function, NULL);
        if ( create_status != 0){
        	printf("Exception On Thread Creation : Code %d\n", create_status);
        	return 1;
        }
    }
    //D: Creates print thread
    create_status = pthread_create(&print_thread, NULL, &print_function, NULL);
    if ( create_status != 0){
       	printf("Exception On Thread Creation : Code %d\n", create_status);
       	return 1;
    }
    //B: Makes sure that all generator threads has finished before proceeding
    for ( i=0; i<PRODUCER_NO; i++ ){
    	join_status = pthread_join(tid[i], NULL);
        if ( join_status != 0){
    		fprintf(stderr, "Exception On Thread Join : Code %d\n", join_status );    
    		return 2;
        }
    }
    //E: Makes sure that print thread has finished before proceeding
    join_status = pthread_join(print_thread, NULL);
    if ( join_status != 0){
    	fprintf(stderr, "Exception On Thread Join : Code %d\n", join_status );
    	return 2;
    }
    pthread_mutex_destroy(&lock);

    return (0);

}

void *generator_function(void *arg) {

    long counter = 0;
    long sum_this_generator = 0;

    while (counter < NUM_PRODUCED) {
        // critical section start
        pthread_mutex_lock(&lock);
        long tmpNumber = sum;
        long rnd_number = rand() % 10;
        printf("current sum of the generated number up to now is %ld going to add %ld to it.\n", tmpNumber, rnd_number);
        sum = tmpNumber + rnd_number;
        // critical section end
        pthread_mutex_unlock(&lock);
        counter++;
        sum_this_generator += rnd_number;
        usleep(500);
    }
    printf("--+---+----+----------+---------+---+--+---+------+----\n");
    printf("The sum of produced items for this number generator at the end is: %ld \n", sum_this_generator);
    printf("--+---+----+----------+---------+---+--+---+------+----\n");
    
    // critical section start
    pthread_mutex_lock(&lock);
    finished_producers++;
    //H: If all generator has finished fire signal for condition variable
    if (finished_producers == PRODUCER_NO)
        pthread_cond_signal(&all_done);
    // critical section end
    pthread_mutex_unlock(&lock);
    return (0);
}

void *print_function(void *arg) {
    //G: Wait until all generator has finished
    // critical section start
    pthread_mutex_lock(&lock);
    while ( finished_producers != PRODUCER_NO )
        pthread_cond_wait(&all_done, &lock);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("The value of counter at the end is: %ld \n", sum);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    // critical section end
    pthread_mutex_unlock(&lock);
}
