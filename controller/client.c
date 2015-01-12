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
#include <linux/reboot.h>
#include "commands.h"



#define BUFSIZE 4096
#define TIME_FOR_COMPLETING_PENDING_REQUESTS 10
char send_buff[BUFSIZE];
char recv_buff[BUFSIZE];

struct timeval initial_time;
time_t now;
void get_features(char *output){
	FILE *t;
	char *command;
	int line; char ch;
	int mem_total, mem_used, mem_free, mem_shared, mem_buffers, mem_cached;
	int swap_total, swap_used, swap_free;
	float cpu_user,cpu_nice, cpu_system, cpu_iowait, cpu_steal, cpu_idle;
	struct timeval curr_time;

	char num_th[128];
	FILE *pof;

	pof = popen("ps -eLf | grep -v defunct | wc -l", "r");
	if(pof == NULL)
		abort();

	fgets(num_th, sizeof(num_th)-1, pof);


	gettimeofday(&curr_time, NULL);
	sprintf(output, "Datapoint: %f %s", (double)curr_time.tv_sec - initial_time.tv_sec + (double)curr_time.tv_usec / 1000000 - (double)initial_time.tv_usec / 1000000, num_th);

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

	// Replaced the old iostat with top to get accurate CPU usage
	//command = "iostat > cpudata.txt";
	command = "top -d 0.2 -b -n2 | grep \"Cpu(s)\" | tail -n 1 | sed 's/%/ /g' > cpudata.txt";
	system(command);
	t = fopen("cpudata.txt","r");
	/*	for(line = 1;line <= 1;line++){
		int dont_stop = 1;
		while(dont_stop){
			fscanf(t,"%c",&ch);
			if(ch=='\n')dont_stop = 0;
		}
	}
	printf("%s\n", t);*/
	fscanf(t," Cpu(s): %f us, %f sy, %f ni, %f id, %f wa, %f hi, %f si, %f st",&cpu_user,&cpu_system,&cpu_nice,&cpu_idle, &cpu_iowait,&cpu_steal,&cpu_steal,&cpu_steal);
	sprintf(output,"%sCPU: %f %f %f %f %f %f",output,cpu_user,cpu_nice,cpu_system,cpu_iowait,cpu_steal,cpu_idle);

	printf("%s\n",output);
	fclose(t);

	return;
}

void connect_to_server(int *sockfd, char *server_addr, int port){

	struct sockaddr_in server;

	//Create socket
	printf("Creating socket...");
	fflush(stdout);
	*sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if (*sockfd == -1)
	{
		printf("\nCould not create socket");
	}
	printf("\nSocket created");

	server.sin_addr.s_addr = inet_addr(server_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	printf("\nConnecting to the server...");
	fflush(stdout);

	//Connect to remote server
	if (connect(*sockfd, (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("\nConnect failed");
		fflush(stdout);
		exit(1);
	}
	printf("\nConnected to the server");
	fflush(stdout);
}		


int main(int argc, char **argv) {
	int sockfd;
	int numbytes;
	if (argc!=3) {
		printf("\nUsage: %s ip_server_address tcp_port_number", argv[0]);
		exit(1);
	}
	connect_to_server(&sockfd, argv[1], atoi(argv[2]));
	gettimeofday(&initial_time, NULL);

	while(1) {
		time(&now);
		printf("\nTime: %s", ctime(&now));
		printf("\nCollecting data features and sending to server...");
		fflush(stdout);
		bzero(send_buff, BUFSIZE);
		get_features(send_buff);
		if ((numbytes = send(sockfd, send_buff, BUFSIZE, 0)) == -1) {
			fflush(stdout);
			perror("send");
			break;
		}
		printf("\n%i bytes sent",numbytes);
		printf("\nWaiting for server commands...");
		fflush(stdout);
		bzero(recv_buff, BUFSIZE);
		if ((numbytes = recv(sockfd, recv_buff, BUFSIZE, 0)) == -1) {
			perror("recv");
			break;
		}


		if (recv_buff[0]==REJUVENATE) {
			//wait for completing pending requests
			printf("\nCommand REJUVENETE received");
			printf("\nWaiting %i seconds for completing pending requests before rejuvenation...", TIME_FOR_COMPLETING_PENDING_REQUESTS);
			fflush(stdout);
			sleep(TIME_FOR_COMPLETING_PENDING_REQUESTS);
			printf("\nExecuting reboot...\n");
			fflush(stdout);
			system("reboot");
			exit(0);
		}


		sleep(1);
	}
	close(sockfd);
	return 0;
}
