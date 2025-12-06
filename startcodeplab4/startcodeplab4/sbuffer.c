/**
 * \author {Bert Lagaisse + Diego Vallés}
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
    pthread_mutex_t mutex;
    pthread_cond_t cond_nempty;
};


int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;

	if (pthread_mutex_init(&(*buffer)->mutex, NULL) != 0) {free(*buffer);*buffer = NULL;return SBUFFER_FAILURE;}

    if (pthread_cond_init(&(*buffer)->cond_nempty, NULL) != 0) {pthread_mutex_destroy(&(*buffer)->mutex);free(*buffer);*buffer = NULL;return SBUFFER_FAILURE;}

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {return SBUFFER_FAILURE;}

	pthread_mutex_lock(&(*buffer)->mutex); // locks for critical operation

    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    (*buffer)->tail = NULL;
    pthread_mutex_unlock(&(*buffer)->mutex);

    pthread_mutex_destroy(&(*buffer)->mutex);
    pthread_cond_destroy(&(*buffer)->cond_nempty);
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
	if (data == NULL) return SBUFFER_FAILURE;

	pthread_mutex_lock(&buffer->mutex);

    while (buffer->head == NULL) {
        pthread_cond_wait(&buffer->cond_nempty, &buffer->mutex);
    }
    *data = buffer->head->data;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) {
        buffer->head = buffer->tail = NULL;
    } else {
        buffer->head = buffer->head->next;
    }

    pthread_mutex_unlock(&buffer->mutex);
    free(dummy);
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
    dummy = malloc(sizeof(sbuffer_node_t));

    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;

	pthread_mutex_lock(&buffer->mutex);

    if (buffer->tail == NULL){ // buffer empty (buffer->head should also be NULL
        buffer->head = buffer->tail = dummy;
    } else { // buffer not empty
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
	pthread_cond_signal(&buffer->cond_nempty);
    pthread_mutex_unlock(&buffer->mutex);

    return SBUFFER_SUCCESS;
}
