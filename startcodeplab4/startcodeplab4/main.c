/**
* \author {Diego Vallés}
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "sbuffer.h"

static sbuffer_t *Buffer = NULL;
static FILE *fout = NULL;
static pthread_mutex_t file_mutex;

void *writef(void *arg){
    FILE *fin = fopen("sensor_data", "rb");
    if (!fin) {
        fprintf(stderr, "open binary file error\n");
        sensor_data_t eos = {0, 0.0, 0};
        for (int i = 0; i < 2; i++) {
            sbuffer_insert(Buffer, &eos);
        }
        return NULL;
    }

    sensor_data_t data;
    while (fread(&data, sizeof(sensor_data_t), 1, fin) == 1) {
        sbuffer_insert(Buffer, &data);
        usleep(10000);
    }
    fclose(fin);

    sensor_data_t eos = {0, 0.0, 0};
    for (int i = 0; i < 2; i++) {
        sbuffer_insert(Buffer, &eos);
    }
    return NULL;
}

void *readf(void *arg){
    while (1) {
        sensor_data_t data;
        if (sbuffer_remove(Buffer, &data) != SBUFFER_SUCCESS) {break;}
        if (data.id == 0) {break;}

        pthread_mutex_lock(&file_mutex);
        fprintf(fout, "%u,%.2f,%ld\n", (unsigned int)data.id, data.value, (long)data.ts);
        pthread_mutex_unlock(&file_mutex);
        usleep(25000);
    }
    return NULL;
}

int main(void){
    if (sbuffer_init(&Buffer) != SBUFFER_SUCCESS) {fprintf(stderr, "sbuffer_init failed\n"); return EXIT_FAILURE;}

    fout = fopen("sensor_data_out.csv", "w");
    if (!fout) {fprintf(stderr,"sensor_data_out.csv\n");sbuffer_free(&Buffer);return EXIT_FAILURE;}

    if (pthread_mutex_init(&file_mutex, NULL) != 0) {fprintf(stderr, "file mutex init failed\n");fclose(fout);sbuffer_free(&Buffer);return EXIT_FAILURE;}

    pthread_t writer;
    pthread_t readers[2];

    pthread_create(&writer, NULL, writef, NULL);
    pthread_create(&readers[0], NULL, readf, NULL);
    pthread_create(&readers[1], NULL, readf, NULL);

    pthread_join(writer, NULL);
    pthread_join(readers[0], NULL);
    pthread_join(readers[1], NULL);

    pthread_mutex_destroy(&file_mutex);
    fclose(fout);
    sbuffer_free(&Buffer);

    return EXIT_SUCCESS;
}
