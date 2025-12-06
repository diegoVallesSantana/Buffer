/**
* \author {AI tool} used to see inside sensor data for debugging
*/
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

int main(void) {
FILE *fp = fopen("sensor_data", "rb");
if (!fp) {
perror("sensor_data");
return EXIT_FAILURE;
}

sensor_data_t data;
int count = 0;

while (fread(&data, sizeof(sensor_data_t), 1, fp) == 1) {
printf("%3d: id=%5u value=%10.6f ts=%ld\n",
count,
(unsigned int)data.id,
data.value,
(long)data.ts);
count++;
}

printf("Total records read: %d\n", count);

fclose(fp);
return EXIT_SUCCESS;
}
