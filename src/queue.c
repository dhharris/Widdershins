/**
 * Implementation of a thread safe queue
 * @author dhharri2
 *
 * Terrible Threads Lab
 * CS 241 - Spring 2016
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../include/queue.h"

extern volatile sig_atomic_t running; /* from crawler.c */

/**
 *  Given data, place it on the queue.  Can be called by multiple threads.
 */
void queue_push(queue_t *queue, void *data) {
    /* Your code here */

    /* Lock the mutex */
    pthread_mutex_lock(&queue->m);


    /* Block if the size is equal to maxSize.
     * Never block for a negative maxSize
     */
    while (running && (queue->maxSize > 0) && (queue->size >= queue->maxSize))
        pthread_cond_wait(&queue->cv, &queue->m);

    /* Add the data */
    queue_node_t *qn = malloc(sizeof(queue_node_t));
    qn->data = data;
    qn->next = NULL;

    if (!queue->head && !queue->tail)
        queue->head = queue->tail = qn;
    else {
        queue->tail->next = qn;
        queue->tail = qn;
    }

    /* increment size */
    ++queue->size;

    /* Signal the condition variable */
    pthread_cond_signal(&queue->cv);

    /* Unlock the mutex */
    pthread_mutex_unlock(&queue->m);
}

/**
 *  Retrieve the data from the front of the queue.  Can be called by multiple
 * threads.
 *  Blocks the queue is empty
 */
void *queue_pull(queue_t *queue) {
    /* Your code here */
    void *data = NULL;
    queue_node_t *qn;

    /* Lock the mutex */
    pthread_mutex_lock(&queue->m);

    /* Empty queue */
    while(running && !queue->size)
        /* Wait on the condition variable */
        pthread_cond_wait(&queue->cv, &queue->m);

    /* Non-empty queue */
    if (queue->size) {
        /* dequeue and decrement size */
        qn = queue->head;
        --queue->size;

        /* We're dequeueing the last element */
        if (queue->head == queue->tail)
            queue->head = queue->tail = NULL;
        else        /* Move the head pointer */
            queue->head = qn->next;
        data = qn->data;

        /* Free the node we dequeue'd */
        free(qn);
    }

    /* Signal the condition variable */
    pthread_cond_signal(&queue->cv);

    /* Unlock the mutex */
    pthread_mutex_unlock(&queue->m);

    /* return data. will be NULL if the queue was empty */
    return data;
}

/**
 *  Initializes the queue
 */
void queue_init(queue_t *queue, int maxSize) {
    /* Your code here */

    /* Set fields */
    queue->maxSize = maxSize;
    queue->size = 0;
    queue->head = queue->tail = NULL;

    /* Initialize condition variable */
    pthread_cond_init(&queue->cv, NULL);

    /* Initialize mutex */
    pthread_mutex_init(&queue->m, NULL);
}

/**
 *  Destroys the queue, freeing any remaining nodes in it.
 */
void queue_destroy(queue_t *queue) {
    /* Your code here */

    /* walk down the list and free variables */
    queue_node_t *p = queue->head;

    while(p) {
        queue_node_t *freevar = p;
        p = p->next;
        free(freevar);
    }

    /* Destroy condition variable */
    pthread_cond_destroy(&queue->cv);

    /* Destroy mutex */
    pthread_mutex_destroy(&queue->m);
}
