#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include "commands.h"

#define TTC_THRESHOLD 50
#define COMMUNICATION_TIMEOUT 60
#define NUMBER_OF_VMs 2


#define BUFSIZE 4096
#define IN_CONN_BACKLOG_LEN 1024
char send_buff[BUFSIZE];
char recv_buff[BUFSIZE];
int current_vm_data_set_index;
struct timeval communication_timeout;
time_t now;



#define MEM_USED_COEF 0.000019237506986
#define MEM_FREE_COEF 0.000236944027358

enum vm_state {
	STAND_BY, ACTIVE, RENJUVUNATING,
};

struct vm_data {
	int socket;
	char ip_address[16];
	enum vm_state state;
};

struct vm_data vm_data_set[NUMBER_OF_VMs];
int active_machine_index = -1;

typedef struct {
	int mem_total;
	int mem_used;
	int mem_free;
	int mem_shared;
	int mem_buffers;
	int mem_cached;

	int swap_total;
	int swap_used;
	int swap_free;

	float cpu_user;
	float cpu_nice;
	float cpu_system;
	float cpu_iowait;
	float cpu_steal;
	float cpu_idle;
} sys_features; //system features

float get_predicted_time_to_crash(char *features) {
	FILE *feat;
	char line[1024];
	int stage;
	int mem_total;
	int mem_used;
	int mem_free;
	int mem_shared;
	int mem_buffers;
	int mem_cached;
	int swap_total;
	int swap_used;
	int swap_free;
	float cpu_user;
	float cpu_nice;
	float cpu_system;
	float cpu_iowait;
	float cpu_steal;
	float cpu_idle;
	float time;

	sscanf(features, "Datapoint: %f\nMemory: %d %d %d %d %d %d\nSwap: %d %d %d\nCPU: %f %f %f %f %f %f", &time,
			&mem_total,&mem_used,&mem_free,&mem_shared,&mem_buffers,&mem_cached,
			&swap_total,&swap_used,&swap_free,
			&cpu_user,&cpu_nice,&cpu_system,&cpu_iowait,&cpu_steal,&cpu_idle);


	//if ((double)rand() < (double)RAND_MAX/(double)4) return TTC_THRESHOLD -1;
	//else return TTC_THRESHOLD+1;
	//return 0;
	return MEM_USED_COEF*mem_used+ MEM_FREE_COEF*mem_free;
}



/*
 * This function look at the incoming requests
 * and if there is at least one request from every connected VM then
 * sends commands to VMs.
 */

void configure_load_balancer(){
	int current_index=0;
	char startcommand[1024];
	bzero(startcommand, 1024);
	printf("\nReconfiguring load balancer...");
	strcat(startcommand, "killall xr; ");
	strcat(startcommand, "/usr/sbin/xr --verbose --server tcp:0:8080 " );
	printf("\nCurrent states");
	printf("\nIP address\tstatus");
	while (current_index<NUMBER_OF_VMs) {
		if (vm_data_set[current_index].state==ACTIVE) {
		strcat(startcommand, "--backend " );
		strcat(startcommand, vm_data_set[current_index].ip_address);
		strcat(startcommand, ":8080 ");
		}
		printf("\n%s\t%i", vm_data_set[current_index].ip_address, vm_data_set[current_index].state);
		fflush(stdout);
		current_index++;
	}

	strcat(startcommand, "--web-interface 0:8001 > /dev/null 2>&1 &" );
	//printf("\nExecuting command \"killall xr &\"... ");
	//fflush(stdout);
	//getchar();
	//system("killall xr");
	printf("\nExecuting command \"%s\"... ",startcommand);
	fflush(stdout);
	//getchar();
	system(startcommand);
};

void *processing_thread(void *arg) {
	int vms_status_chenged=0;
	printf("\nProcessing thread started");
	printf("\n%i machines now connected", current_vm_data_set_index);
	fflush(stdout);

	current_vm_data_set_index=0;
	configure_load_balancer();

	int numbytes;
	while (1) {
		sleep(1);
		time(&now);
		printf("\nTime: %s", ctime(&now));
		current_vm_data_set_index++;
		if (current_vm_data_set_index==NUMBER_OF_VMs) current_vm_data_set_index=0;
		vms_status_chenged=0;
		if (vm_data_set[current_vm_data_set_index].state==ACTIVE) {
			printf("\nWaiting feature data from machine with IP address %s index %i", vm_data_set[current_vm_data_set_index].ip_address, current_vm_data_set_index);
			fflush(stdout);
			//getchar();
			// receiving data from client
			bzero(recv_buff, BUFSIZE);
			if ((numbytes = recv(vm_data_set[current_vm_data_set_index].socket, recv_buff, BUFSIZE, 0)) == -1) {
				printf("\nFailed receiving data from machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
				fflush(stdout);
			} else  if (numbytes == 0) {
				printf("\nMachine with IP address %s is disconnected", vm_data_set[current_vm_data_set_index].ip_address);
				fflush(stdout);
			} else  if (numbytes == -1) {
				if (errno == EWOULDBLOCK) {
					printf("\nTimeout on recv() while waiting data from machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
					fflush(stdout);
				} else {
					printf("\nError on recv() while waiting data from machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
					fflush(stdout);
				}
				continue;
			} else {
				printf("\nReceived data (%i bytes) from machine with IP address %s index %i", numbytes, vm_data_set[current_vm_data_set_index].ip_address, current_vm_data_set_index);
				printf("\n%s", recv_buff);
				fflush(stdout);
				float predicted_time_to_crash =get_predicted_time_to_crash(recv_buff);
				printf("\nPredicted time to crash for machine with IP address %s: %f", vm_data_set[current_vm_data_set_index].ip_address, predicted_time_to_crash);
				fflush(stdout);
				if (predicted_time_to_crash<(float)TTC_THRESHOLD) {

					//sending soft-rejuvenation command to the vm
					bzero(send_buff, BUFSIZE);
					send_buff[0] = REJUVENATE;
					if ((send(vm_data_set[current_vm_data_set_index].socket, send_buff, BUFSIZE,
							0)) == -1) {
						perror("send");
						break;
					}
					printf("\nREJUVENATE command sent to machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
					fflush(stdout);
					vms_status_chenged=1;
					// switch active vm
					int vm_data_set_index_to_activate;
					vm_data_set[current_vm_data_set_index].state=STAND_BY;
					if (current_vm_data_set_index %  2== 0)
						vm_data_set_index_to_activate= current_vm_data_set_index+1;
					else
						vm_data_set_index_to_activate= current_vm_data_set_index-1;
					vm_data_set[vm_data_set_index_to_activate].state=ACTIVE;
					printf("\nMachine with IP address %s is now active",
							vm_data_set[vm_data_set_index_to_activate].ip_address);
				} else {
					//sending CONTINUE command to the vm
					bzero(send_buff, BUFSIZE);
					send_buff[0] = CONTINUE;
					if ((send(vm_data_set[current_vm_data_set_index].socket, send_buff, BUFSIZE,0)) == -1) {
						if (errno == EWOULDBLOCK) {
							printf("\nTimeout on send() while sending data to machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
							fflush(stdout);
						} else {
							printf("\nError on send() while sending data to machine with IP address on recv while waiting data from machine with IP address %s", vm_data_set[current_vm_data_set_index].ip_address);
							fflush(stdout);
						}
						continue;
					}

				}
			}
			if (vms_status_chenged)	configure_load_balancer();
		}
	}
}

/*
 * Listens for incoming connection requests. When such arrives this void
 * fires a new thread to handle it.
 * sockfd - the socketd address of the server
 */

void accept_new_clients(int sockfd) {
	unsigned int addr_len;
	int socket;

	printf("\nAccepting connection from vms...");
	fflush(stdout);
	struct sockaddr_in client_addr;
	addr_len = sizeof(struct sockaddr_in);

	while (current_vm_data_set_index<NUMBER_OF_VMs) {
		if ((socket = accept(sockfd, (struct sockaddr *) &client_addr, &addr_len)) == -1) {
			perror("\nAccept failed");
			fflush(stdout);
		}
		/*
	printf("\nAccepted connection from IP address: %s port: %d",
			inet_ntoa(client_addr.sin_addr), (int) ntohs(client_addr.sin_port));
	fflush(stdout);
		 */

		strcpy(vm_data_set[current_vm_data_set_index].ip_address, inet_ntoa(client_addr.sin_addr));
		vm_data_set[current_vm_data_set_index].socket = socket;

		// set timeouts
		if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			error("\nSetsockopt failed for socket_id %i", socket);
		if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			error("\nSetsockopt failed for socket_id %i", socket);


		printf("\nHost with IP address %s added as ", inet_ntoa(client_addr.sin_addr));
		if (current_vm_data_set_index % 2 == 0) {
			vm_data_set[current_vm_data_set_index].state = ACTIVE;
			printf("primary ");
		} else {
			vm_data_set[current_vm_data_set_index].state = STAND_BY;
			printf("backup ");
		}

		printf("in position %i with socket_id %i", current_vm_data_set_index, socket);
		fflush(stdout);
		current_vm_data_set_index++;
	}
}

void accept_disconnect_clients(int sockfd) {
	unsigned int addr_len;
	int socket;

	struct sockaddr_in client_addr;
	addr_len = sizeof(struct sockaddr_in);

	while (1) {
		if ((socket = accept(sockfd, (struct sockaddr *) &client_addr, &addr_len)) == -1) {
			perror("\nAccept failed");
			fflush(stdout);
		}

		// find the position in the vm_data_set
		int current_index;
		for (current_index=0; current_index<NUMBER_OF_VMs; current_index++) {
			//printf("\nHost %s host2 %s ",vm_data_set[current_index].ip_address, inet_ntoa(client_addr.sin_addr));
			fflush(stdout);
			if (strcmp(vm_data_set[current_index].ip_address, inet_ntoa(client_addr.sin_addr))==0)
				break;
		}

		if (current_index==NUMBER_OF_VMs) {
			printf("\nImpossible to add machine with IP address %s", inet_ntoa(client_addr.sin_addr));
			continue;
		}

		vm_data_set[current_index].socket = socket;

		// set timeouts
		if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			printf("\nSetsockopt failed for socket_id %i", socket);
		if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			printf("\nSetsockopt failed for socket_id %i", socket);


		printf("\nHost with IP address %s is again connected", inet_ntoa(client_addr.sin_addr));
		fflush(stdout);

	}
}

void start_server(int *sockfd_addr, int port) {
	int sock;
	struct sockaddr_in temp;
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	//Server address
	temp.sin_family = AF_INET;
	temp.sin_addr.s_addr = INADDR_ANY;
	temp.sin_port = htons(port);
	//Bind
	if (bind(sock, (struct sockaddr*) &temp, sizeof(temp))) {
		perror("\nError on binding");
		exit(1);
	}
	if (listen(sock, IN_CONN_BACKLOG_LEN)) {
		perror("\nError on listening");
		exit(1);
	}
	*sockfd_addr = sock;
}

int main(int argc, char **argv) {

	int sockfd;
	// set receive timeout for clients
	communication_timeout.tv_sec = COMMUNICATION_TIMEOUT;
	communication_timeout.tv_usec = 0;

	memset(vm_data_set, 0, sizeof(struct vm_data) * NUMBER_OF_VMs);

	//Accept vms
	start_server(&sockfd, atoi(argv[1]));
	accept_new_clients(sockfd);

	// Run the processing thread
	pthread_t processing_thread_ptr;
	pthread_attr_t pthread_custom_attr;
	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&processing_thread_ptr, &pthread_custom_attr, processing_thread, 0);

	//Accept disconnect vms
	accept_disconnect_clients(sockfd);

	return 0;
}
