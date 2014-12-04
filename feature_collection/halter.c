#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CONSECUTIVE_TIMES	4

int when_to_break;
int mem_threshold;

float new_datapoint;
float last_datapoint = 0;
int mem_used;
int last_mem_used = 0;

int consecutive_times = 0;

int consecutive_times_for_memory = 0;

int dummy;
int flag;

int main (int argc, char **argv) {
	size_t length = 128;
	char *line = malloc(length);

	if(argc < 3) {
		printf("Usage: %s when_to_break mem_threshold\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	when_to_break = atoi(argv[1]);
	mem_threshold = atoi(argv[2]);

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

			if(strncmp(line, "Memory", strlen("Memory")) == 0) {
				sscanf(line,"Memory: %d %d %d %d %d %d\n",&dummy,&mem_used,&dummy,&dummy,&dummy,&dummy);

				printf("Memory used is %d, tracking is %s\n", mem_used, (flag ? "active" : "inactive"));

				if(mem_used > mem_threshold)
					flag = 1;

				if(flag) {
					if((double)mem_used < 0.8 * (double)last_mem_used) {
						printf("stopping for memory usage drop (mem_used: %f, last_mem_used: %f\n", (double)mem_used, (double)last_mem_used);
						exit(EXIT_SUCCESS);
					}

				}

				if(mem_used > 1900000) {
					fflush(stdout);
					consecutive_times_for_memory++;
				}

				if(consecutive_times_for_memory == 200) {
					printf("Mem used > 1900000 for 200 seconds, exiting\n");
					exit(0);
				}

				last_mem_used = mem_used;
			}
		}
	}

	return 0;
}
