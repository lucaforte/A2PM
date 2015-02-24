#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


#define BUFSIZE 4096


int num_cpus;

char send_buff[BUFSIZE];

struct timeval initial_time;

void get_features(char *output){
	FILE *t;
	char *command;
	int line; char ch;
	int mem_total, mem_used, mem_free, mem_shared, mem_buffers, mem_cached;
	int swap_total, swap_used, swap_free;
	unsigned long cpu_user1, cpu_nice1, cpu_system1, cpu_iowait1, cpu_steal1, cpu_idle1, dummyA1, dummyB1, dummyC1, dummyD1;
	unsigned long cpu_user2, cpu_nice2, cpu_system2, cpu_iowait2, cpu_steal2, cpu_idle2, dummyA2, dummyB2, dummyC2, dummyD2;
	float cpu_user, cpu_nice, cpu_system, cpu_iowait, cpu_steal, cpu_idle;
	float cpu_total;
	struct timeval curr_time;

	char num_th[128];
	FILE *pof, *fstat;

	/* Get number of active threads */
	pof = popen("ps -eLf | grep -v defunct | wc -l", "r");
	if(pof == NULL)
		abort();

	fgets(num_th, sizeof(num_th)-1, pof);
	pclose(pof);
	

	/* Get timestamp */
	gettimeofday(&curr_time, NULL);
	sprintf(output, "Datapoint: %f %s", (double)curr_time.tv_sec - initial_time.tv_sec + (double)curr_time.tv_usec / 1000000 - (double)initial_time.tv_usec / 1000000, num_th);



	/* Get memory and swap usage */
	command = "free > memdata.txt";
	system(command);
	t = fopen("memdata.txt","r");
	for(line = 1;line<= 1;line++){
		int dont_stop = 1;
		while(dont_stop){
			fscanf(t,"%c",&ch);
			if(ch=='\n') dont_stop = 0;
		}
	}
	fscanf(t,"Mem:\t%d\t%d\t%d\t%d\t%d\t%d\n",&mem_total,&mem_used,&mem_free,&mem_shared,&mem_buffers,&mem_cached);
	sprintf(output,"%sMemory: %d %d %d %d %d %d\n",output,mem_total,mem_used,mem_free,mem_shared,mem_buffers,mem_cached);
	for(line = 1;line<= 1;line++){
		int dont_stop = 1;
		while(dont_stop){
			fscanf(t,"%c",&ch);
			if(ch=='\n') dont_stop = 0;
		}
	}
	fscanf(t,"Swap:\t%d\t%d\t%d",&swap_total,&swap_used,&swap_free);
	sprintf(output,"%sSwap: %d %d %d\n",output,swap_total,swap_used,swap_free);	
	fclose(t);
	

	/* Get CPU Usage (using /proc/stat) */
	fstat = fopen("/proc/stat", "r");
	if (fstat == NULL) {
		perror("FOPEN ERROR ");
		exit(EXIT_FAILURE);
	}
	if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_user1, &cpu_nice1, &cpu_system1, &cpu_idle1, &cpu_iowait1, &dummyA1, &dummyB1, &cpu_steal1, &dummyC1, &dummyD1) == EOF) {
		exit(EXIT_FAILURE);
	}
	fclose(fstat);
	sleep(1);
	fstat = fopen("/proc/stat", "r");
        if (fstat == NULL) {
                perror("FOPEN ERROR ");
                exit(EXIT_FAILURE);
        }
        if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_user2, &cpu_nice2, &cpu_system2, &cpu_idle2, &cpu_iowait2, &dummyA2, &dummyB2, &cpu_steal2, &dummyC2, &dummyD2) == EOF) {
                exit(EXIT_FAILURE);
        }
        fclose(fstat);

	cpu_total = (float)(cpu_user2 + cpu_nice2 + cpu_system2 + cpu_idle2 + cpu_iowait2 + dummyA2 + dummyB2 + cpu_steal2 + dummyC2 + dummyD2);
	cpu_total -= (float)(cpu_user1 + cpu_nice1 + cpu_system1 + cpu_idle1 + cpu_iowait1 + dummyA1 + dummyB1 + cpu_steal1 + dummyC1 + dummyD1);

	cpu_user = (float)(cpu_user2 - cpu_user1) * 100.0 / cpu_total;
	cpu_system = (float)(cpu_system2 - cpu_system1) * 100.0 / cpu_total;
	cpu_nice = (float)(cpu_nice2 - cpu_nice1) * 100.0 / cpu_total;
	cpu_idle = (float)(cpu_idle2 - cpu_idle1) * 100.0 / cpu_total;
	cpu_iowait = (float)(cpu_iowait2 - cpu_iowait1) * 100.0 / cpu_total;
	cpu_steal = (float)(cpu_steal2 - cpu_steal1) * 100.0 / cpu_total;

	sprintf(output,"%sCPU: %f %f %f %f %f %f", output, cpu_user, cpu_nice, cpu_system, cpu_iowait, cpu_steal, cpu_idle);

	printf("%s\n",output);
	
	return;
}


void connect_to_server(int *sockfd, char *server_addr){

    struct sockaddr_in server;

    //Create socket
    *sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (*sockfd == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr(server_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(4444);

    //Connect to remote server
    if (connect(*sockfd, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        exit(1);
    }
	printf("Connected\n");	
}		


int main(int argc, char **argv) {
	int sockfd;
	int numbytes;
	connect_to_server(&sockfd, argv[1]);
	gettimeofday(&initial_time, NULL);

	num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	
	while(true) {	
		bzero(send_buff, BUFSIZE);
		get_features(send_buff);
		if ((numbytes = send(sockfd, send_buff, BUFSIZE, 0)) == -1) {
			perror("send");
		   	break;
		}
		usleep(1000000);
	}
	close(sockfd);
	return 0;
}
