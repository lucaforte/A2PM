#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CONSECUTIVE_TIMES	5

int when_to_break;
int swap_threshold;

float new_datapoint;
float last_datapoint = 0;
int swap_used;
int mem_used;
int swap_used;

int consecutive_times = 0;

int dummy;

int main (int argc, char **argv) {
	size_t length = 128;
	char *line = malloc(length);

	if(argc < 3) {
		printf("Usage: %s when_to_break swap_threshold\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	when_to_break = atoi(argv[1]);
	swap_threshold = atoi(argv[2]);

	printf("Breaking when generation difference is higher than %d\n", when_to_break);

	while (1) {

		if (getline(&line, &length, stdin) != -1) {
			if(strncmp(line, "Datapoint", strlen("Datapoint")) == 0) {
				sscanf(line, "Datapoint: %f\n", &new_datapoint);
				printf("Read datapoint at stamp %f,", new_datapoint);
				printf(" difference is %f, ", new_datapoint - last_datapoint);

				if(new_datapoint - last_datapoint > (double)when_to_break) {

					consecutive_times++;

					if(consecutive_times > CONSECUTIVE_TIMES) {
						printf("stopping for too high datapoint generation time\n");
						exit(EXIT_SUCCESS);
					}
				} else {
					consecutive_times = 0;
				}

				printf("continuing\n");

				last_datapoint = new_datapoint;
			}

			if(strncmp(line, "Swap", strlen("Swap")) == 0) {
				sscanf(line, "Swap: %d %d %d\n", &dummy, &swap_used, &dummy);

				printf("Used swap is %d\n", swap_used);

				if(swap_used > swap_threshold) {
					printf("stopping for too high swap usage\n");
					exit(EXIT_SUCCESS);
				}
			}
		}
	}

	return 0;
}
