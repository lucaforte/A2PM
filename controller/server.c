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
#include "ml_models.h"
#include "system_features.h"
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define TTC_THRESHOLD 50
#define COMMUNICATION_TIMEOUT 60
#define NUMBER_OF_VMs 2

#define BUFSIZE 4096
#define IN_CONN_BACKLOG_LEN 1024

#define SWAP_FAILURE_THRESHOLD 450000
#define RESP_TIME_FAILURE_THRESHOLD 8
#define MAX_CONSEC_FAILURE_ADMITTED 5

char send_buff[BUFSIZE];
char recv_buff[BUFSIZE];
int current_vm_data_set_index;
struct timeval communication_timeout;
int ml_model;
time_t now;
int rejuvenation_counter;
int consecutive_swap_failure_counter;
int consecutive_response_time_failure_counter;
int response_time_failure_counter;
int swap_failure_counter;
system_features current_features;
FILE *output_file;


enum vm_state {
	STAND_BY, ACTIVE, RENJUVUNATING,
};

struct vm_data {
	int socket;
	char ip_address[16];
	enum vm_state state;
	system_features last_features;
	int last_system_features_stored;
};

volatile struct vm_data vm_data_set[NUMBER_OF_VMs];
int active_machine_index = -1;

void get_feature(char *features) {
	sscanf(features, "Datapoint: %f %d\nMemory: %d %d %d %d %d %d\nSwap: %d %d %d\nCPU: %f %f %f %f %f %f",
			&current_features.time, &current_features.n_th,
			&current_features.mem_total,&current_features.mem_used,&current_features.mem_free,&current_features.mem_shared,&current_features.mem_buffers,&current_features.mem_cached,
			&current_features.swap_total,&current_features.swap_used,&current_features.swap_free,
			&current_features.cpu_user,&current_features.cpu_nice,&current_features.cpu_system,&current_features.cpu_iowait,&current_features.cpu_steal,&current_features.cpu_idle);
}

void store_last_system_features(system_features *last_features) {
	memcpy(last_features, &current_features, sizeof(system_features));
}

int machine_failed(system_features last_features, system_features current_features) {
        // swap
        if (current_features.swap_used>(float)SWAP_FAILURE_THRESHOLD) {
                printf("\nFalse negative suspected: swap: %d (threshold: %d)",current_features.swap_used, SWAP_FAILURE_THRESHOLD, current_features.time-last_features.time, RESP_TIME_FAILURE_THRESHOLD);
                consecutive_swap_failure_counter++;
        } else {
                        consecutive_swap_failure_counter=0;
        }
        if (consecutive_swap_failure_counter > MAX_CONSEC_FAILURE_ADMITTED) {
                printf("\nFalse negative detected: swap: %d (threshold: %d)",current_features.swap_used, SWAP_FAILURE_THRESHOLD, current_features.time-last_features.time, RESP_TIME_FAILURE_THRESHOLD);
                consecutive_swap_failure_counter=0;
                swap_failure_counter++;
                return 1;
        }

        //response_time
        if (current_features.time-last_features.time>RESP_TIME_FAILURE_THRESHOLD) {
                printf("\nFalse negative suspected: interarrival time: %f (threshold: %d)", current_features.time-last_features.time, RESP_TIME_FAILURE_THRESHOLD);
                consecutive_response_time_failure_counter++;
        } else {
                consecutive_response_time_failure_counter=0;
        }
        if (consecutive_response_time_failure_counter > MAX_CONSEC_FAILURE_ADMITTED) {
                printf("\nFalse negative detected: interarrival time: %f (threshold: %d)", current_features.time-last_features.time, RESP_TIME_FAILURE_THRESHOLD);
                consecutive_response_time_failure_counter=0;
                response_time_failure_counter++;
                return 1;
        }
        return 0;
}


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

void switch_active_machine() {
	// switch active vm
	int vm_data_set_index_to_activate;
	vm_data_set[current_vm_data_set_index].state = STAND_BY;
	if (current_vm_data_set_index % 2 == 0)
		vm_data_set_index_to_activate = current_vm_data_set_index + 1;
	else
		vm_data_set_index_to_activate = current_vm_data_set_index - 1;
	vm_data_set[vm_data_set_index_to_activate].state = ACTIVE;
	printf("\nMachine with IP address %s is now active",
			vm_data_set[vm_data_set_index_to_activate].ip_address);
	//send soft-rejuvenation command to the vm
	bzero(send_buff, BUFSIZE);
	send_buff[0] = REJUVENATE;
	if ((send(vm_data_set[current_vm_data_set_index].socket, send_buff, BUFSIZE,
			0)) == -1) {
		perror("send");
	}
	printf("\nREJUVENATE command sent to machine with IP address %s",
			vm_data_set[current_vm_data_set_index].ip_address);
	fflush(stdout);
}

void processing_thread(void *arg) {
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
                output_file=fopen("output.txt", "w");
                fprintf(output_file, "Rejuvenations: %i\tResponse time failures: %i\tSwap failures: %i\tTime: %s", rejuvenation_counter, response_time_failure_counter, swap_failure_counter, ctime(&now));
                fclose(output_file);
		current_vm_data_set_index++;
		if (current_vm_data_set_index==NUMBER_OF_VMs) current_vm_data_set_index=0;
		vms_status_chenged=0;
		if (vm_data_set[current_vm_data_set_index].state==ACTIVE) {
			//sending WAITING_DATA command to the vm
			printf("\nSending WAITING_DATA command to machine with IP address %s index %i", vm_data_set[current_vm_data_set_index].ip_address, current_vm_data_set_index);
			fflush(stdout);
			bzero(send_buff, BUFSIZE);
			send_buff[0] = WAITING_DATA;
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
				get_feature(recv_buff);
				if (vm_data_set[current_vm_data_set_index].last_system_features_stored) {
					//checking false negative
					if (machine_failed(vm_data_set[current_vm_data_set_index].last_features, current_features)) {
						fflush(stdout);
	                    switch_active_machine();
	                    configure_load_balancer();
	                	vm_data_set[current_vm_data_set_index].last_system_features_stored = 0;
						continue;
					}
					//does the machine need to be rejuveneted?
					float predicted_time_to_crash = get_predicted_rttc(ml_model, vm_data_set[current_vm_data_set_index].last_features, current_features);
					printf("\nPredicted time to crash for machine with IP address %s: %f", vm_data_set[current_vm_data_set_index].ip_address, predicted_time_to_crash);
					fflush(stdout);
					if (predicted_time_to_crash<(float)TTC_THRESHOLD) {
						//yes, the machine need to be rejuveneted
						switch_active_machine();
						configure_load_balancer();
						vm_data_set[current_vm_data_set_index].last_system_features_stored = 0;
						rejuvenation_counter++;
						continue;
					}
				}
				store_last_system_features(&vm_data_set[current_vm_data_set_index].last_features);
				vm_data_set[current_vm_data_set_index].last_system_features_stored=1;
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

	printf("\nAccepting connections...");
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
			error("\nSetsockopt failed for socket id %i", socket);
		if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			error("\nSetsockopt failed for socket id %i", socket);


		printf("\nHost with IP address %s added as ", inet_ntoa(client_addr.sin_addr));
		if (current_vm_data_set_index % 2 == 0) {
			vm_data_set[current_vm_data_set_index].state = ACTIVE;
			printf("primary ");
		} else {
			vm_data_set[current_vm_data_set_index].state = STAND_BY;
			printf("backup ");
		}

		printf("in position %i with socket id %i", current_vm_data_set_index, socket);
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
			printf("\nSetsockopt failed for socket id %i", socket);
		if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
			printf("\nSetsockopt failed for socket id %i", socket);


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
	printf("\nServer started");
	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;

	printf("\nIP addresses are:");
	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("\n%s IP address %s", ifa->ifa_name, addressBuffer);
		}
	}

}

int main(int argc, char **argv) {

	int sockfd;
	int port;
	rejuvenation_counter=0;
	swap_failure_counter=0;
	response_time_failure_counter=0;
	consecutive_swap_failure_counter=0;
	consecutive_response_time_failure_counter=0;
	// set receive timeout for clients
	communication_timeout.tv_sec = COMMUNICATION_TIMEOUT;
	communication_timeout.tv_usec = 0;
	memset(vm_data_set, 0, sizeof(struct vm_data) * NUMBER_OF_VMs);

	if (argc!=3) {
		printf("\nUsage: executable_file_name tcp_port_number ml_model_number\n");
		exit(1);
	}

	port=atoi(argv[1]);
	ml_model =atoi(argv[2]);

	//Accept vms
	start_server(&sockfd, port);
	accept_new_clients(sockfd);

	// Run the processing thread
	pthread_t processing_thread_ptr;
	pthread_attr_t pthread_custom_attr;
	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&processing_thread_ptr, &pthread_custom_attr,processing_thread, 0);

	//Accept disconnect vms
	accept_disconnect_clients(sockfd);

	return 0;
}
