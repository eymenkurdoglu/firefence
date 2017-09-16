/*
 * pthread_sample.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: Kuan-yin Chen
 */

// ---- Includes ----

#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <cstdio>
#include <deque>

// ---- Macros ----
#define N_THREADS 5         // Number of string matching threads.
#define MAX_FIFO_SIZE 1000  // Size of each FIFO queue, in elements.
#define PKT_INTERVAL 10000  // Simulated packet arrival interval, in useconds.
#define UPDATE_INTERVAL 1   // Counter update interval, in seconds
#define RAND_RNG  100
#define THRESHOLD 90
#define PRINT_COUNTER 1

using namespace std;

// ---- Global vars ----
bool stop = 0;

// ---- Define argument data structure ----
typedef struct{
    deque<int>              queue;      // FIFO is implemented with deque
    unsigned long int       n_proc;     // Counter for processed packets
    unsigned long int       n_detd;     // Counter for detected packets
    pthread_mutex_t         lock_queue;     // Mutex lock for queue
    pthread_mutex_t         lock_n_proc;    // Mutex lock for n_proc
    pthread_mutex_t         lock_n_detd;    // Mutex lock for n_detd
    int                     tid;        // Thread ID
}fifo_t;

typedef struct{
    unsigned long int       n_queued;
    unsigned long int       n_detected;
}count_ret_t;

typedef struct{
    unsigned long int       n_captured;
    unsigned long int       n_queued;
    unsigned long int       n_discard;
}pcapt_ret_t;

typedef struct{
    unsigned long int       n_queued;
    unsigned long int       n_detected;
}match_ret_t;

// ---- Prototype of thread functions ----
void * count_func(void * fifos);    // Counter thread function
void * pcapt_func(void * fifos);    // Packet capture thread function
void * match_func(void * fifo);     // String matching thread function

// ---- Main course ----
int main(){
    int         i, res;

    pthread_t   count_thread;
    pthread_t   pcapt_thread;
    pthread_t   match_threads[N_THREADS];

    count_ret_t* count_ret;
    pcapt_ret_t* pcapt_ret;
    match_ret_t* match_rets[N_THREADS];

    fifo_t      fifos[N_THREADS];

    srand(time(NULL));

    // ---- Initialize the fifos ----
    for ( i = 0; i < N_THREADS; i++ ) {
        fifos[i].tid = i;
        fifos[i].n_proc = 0;
        fifos[i].n_detd = 0;
        pthread_mutex_init(&fifos[i].lock_queue, NULL);
        pthread_mutex_init(&fifos[i].lock_n_proc, NULL);
        pthread_mutex_init(&fifos[i].lock_n_detd, NULL);
    }

    // ---- Create threads ----
    // Create a counter thread
    res = pthread_create( &count_thread, NULL, count_func, (void *)fifos );
    if ( res == 0 ) {
        printf("Counter thread is successfully created.\n");
    }

    // Create a packet capture thread
    res = pthread_create(& pcapt_thread, NULL, pcapt_func, (void *)fifos);
    if(res == 0){
        printf("Packet capture thread is successfully created.\n");
    }

    // Create N_THREADS string matching threads
    for( i = 0; i < N_THREADS; i++ ) {
        res = pthread_create(&match_threads[i], NULL, match_func, (void *)&fifos[i]);
        if ( res == 0 ) {
            printf("String matching thread #%d is successfully created.\n", i);
        }
    }

    // ---- Press enter to raise the signal of thread termination ----
    printf("Press ENTER to terminate the threads.\n");
    getchar();
    stop = 1;

    // ---- Wait for all threads to finish ----
    pthread_join(count_thread, (void **) & count_ret);
    pthread_join(pcapt_thread, (void **) & pcapt_ret);
    for( i = 0; i < N_THREADS; i++ ) {
        pthread_join(match_threads[i], (void **) & match_rets[i]);
    }

    // ---- Print results for further checking ----
    printf("========== Results ==========\n\n");
    printf("Packet capture thread:\n");
    printf("  Packets captured = %lu\n", pcapt_ret->n_captured);
    printf("  Packets queued = %lu\n", pcapt_ret->n_queued);
    printf("  Packets discarded = %lu\n", pcapt_ret->n_discard);
    printf("\n");

    for ( i = 0; i < N_THREADS; i++ ) {
        printf("String matching thread #%d:  ", i);
        printf("  Packets processed = %lu  ", match_rets[i]->n_queued);
        printf("  detected = %lu\n", match_rets[i]->n_detected);
    }

    return 0;
}


/*
 *  void * count_func(void * fifos)
 *  The counter thread function.
 */
void * count_func(void * fifos){
    int i;
    count_ret_t * res = (count_ret_t *) malloc(sizeof(count_ret_t));

    // Initialize return data structure
    res->n_queued = 0;
    res->n_detected = 0;

    fifo_t * fptr = (fifo_t *) fifos;

    // Periodically pull counter values from thread FIFOs.
    while(!stop){
        res->n_queued = 0;
        res->n_detected = 0;
        for(i=0 ; i<N_THREADS ; i++){
            pthread_mutex_lock(& fptr[i].lock_n_proc);
            res->n_queued += fptr[i].n_proc;
            pthread_mutex_unlock(& fptr[i].lock_n_proc);

            pthread_mutex_lock(& fptr[i].lock_n_detd);
            res->n_detected += fptr[i].n_detd;
            pthread_mutex_unlock(& fptr[i].lock_n_detd);
        }

        sleep(UPDATE_INTERVAL);     /* Sleep for UPDATE_INTERVAL and then
                                             * re-start value pulling.
                                             */

#ifdef PRINT_COUNTER
        printf("Packets queued = %-15lu  detected = %-15lu\n",
                res->n_queued, res->n_detected);
#endif


    }

    pthread_exit((void *) res);
}

/*
 *  void * pcapt_func(void * fifos)
 *  The packet capture thread function.
 */
void * pcapt_func(void * fifos){
    int rr = 0;     // Round-robin counter
    int pkt = 0;
    pcapt_ret_t * res = (pcapt_ret_t *) malloc(sizeof(pcapt_ret_t));

    // Initialize return data structure
    res->n_captured = 0;
    res->n_queued = 0;
    res->n_discard = 0;

    fifo_t * fptr = (fifo_t *) fifos;   // The pointer to the FIFOs is passed
                                        // to this function as a void pointer.
                                        // Therefore we must cast it back to
                                        // fifo_t pointer.

    while ( !stop ) {
        /*
         *  In the packet capture function, we simulate packet capturing by
         *  generating random integers and assign them to each string matching
         *  thread's FIFO in a round-robin manner. It is up to you to integrate
         *  PCAP interface into this piece of code.
         */
        pkt = rand() % RAND_RNG;
        res->n_captured ++;

        if ( fptr[rr].queue.size() < MAX_FIFO_SIZE ) {
            // Lock the mutex before queuing the packet
            pthread_mutex_lock(& fptr[rr].lock_queue);
            fptr[rr].queue.push_back(pkt);
            pthread_mutex_unlock(& fptr[rr].lock_queue);
            // Unlock the mutex before queuing the packet

            res->n_queued ++;
        }
        else{
            // Abandon packet.
            res->n_discard ++;
        }

        rr = (rr + 1) % N_THREADS;
        usleep(PKT_INTERVAL);   // Simulate packet capture every PKT_INTERVAL useconds.
    }

    pthread_exit((void *) res);
}

/*
 *  void * match_func(void * fifos)
 *  The string matching thread function.
 */
void* match_func( void* fifo ) {
    int pkt = -1;
    match_ret_t* res = (match_ret_t*) malloc( sizeof(match_ret_t) );
    fifo_t* fptr = (fifo_t*)fifo;

    while ( !stop ) {
        if ( !fptr->queue.empty() ) {
            pkt = fptr->queue.front();
            pthread_mutex_lock( &fptr->lock_queue );
            fptr->queue.pop_front();
            pthread_mutex_unlock( &fptr->lock_queue );

            pthread_mutex_lock( &fptr->lock_n_proc );
            fptr->n_proc++;
            pthread_mutex_unlock( &fptr->lock_n_proc );

            /*
             * We simulate pattern detection by checking packets (which are
             * simulated by integers) against a threshold value. It is up to you
             * to integrate string matching algorithm into this piece of code.
             */
            if ( pkt != -1 && pkt > THRESHOLD ) {
                pthread_mutex_lock( &fptr->lock_n_detd );
                fptr->n_detd++;
                pthread_mutex_unlock( &fptr->lock_n_detd );
            }
        }
    }

    res->n_queued = fptr->n_proc;
    res->n_detected = fptr->n_detd;
    
    pthread_exit( (void *)res );
}
