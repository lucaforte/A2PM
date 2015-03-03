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
#include "commands.h"
#include "ml_models.h"
#include "system_features.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>

#define COMMUNICATION_TIMEOUT   60
#define NUMBER_GROUPS           3       // one group for each type of service
#define NUMBER_VMs              5       // number of possible VMs initially for each group
#define CONN_BACKLOG            1024    // max number of pending connections
#define BUFSIZE                 4096    // buffer size (in bytes)
#define TTC_THRESHOLD           300     // threshold to rejuvenate the VM (in sec)


int ml_model;                           // used machine-learning model
struct timeval communication_timeout;
int current_vms[NUMBER_GROUPS];         // number of connected VMs to the server
int allocated_vms[NUMBER_GROUPS];       // number of allocated spaces for the VMs (current_vms[i] <= allocated_vms[i])
pthread_mutex_t mutex;                  // mutex used to update current_vms and allocated_vms values

/*** TODO: initialize with real provided services ***/
// services provided
enum services {
    SERVICE_0, SERVICE_1, SERVICE_2
};

// STAND_BY has value 0, ACTIVE has value 1, REJUVENATING has value 2, and so on...
enum vm_state {
    STAND_BY, ACTIVE, REJUVENATING
};

// service info for each VM
struct vm_service{
    enum services service;
    enum vm_state state;
};

struct vm_data {
    int socket;
    char ip_address[16];
    struct vm_service service_info;
    system_features last_features;
    int last_system_features_stored;
};

struct vm_data * vm_data_set[NUMBER_GROUPS]; //Groups of VMs

/* Fill current_features */
void get_feature(char *features, system_features current_features) {
    sscanf(features, "Datapoint: %f %d\nMemory: %d %d %d %d %d %d\nSwap: %d %d %d\nCPU: %f %f %f %f %f %f",
           &current_features.time, &current_features.n_th,
           &current_features.mem_total,&current_features.mem_used,&current_features.mem_free,&current_features.mem_shared,&current_features.mem_buffers,&current_features.mem_cached,
           &current_features.swap_total,&current_features.swap_used,&current_features.swap_free,
           &current_features.cpu_user,&current_features.cpu_nice,&current_features.cpu_system,&current_features.cpu_iowait,&current_features.cpu_steal,&current_features.cpu_idle);
}

void store_last_system_features(system_features *last_features, system_features current_features) {
    memcpy(last_features, &current_features, sizeof(system_features));
}

void configure_load_balancer(){
    //    int current_index=0;
    //    char startcommand[1024];
    //    bzero(startcommand, 1024);
    //    printf("\nReconfiguring load balancer...");
    //    strcat(startcommand, "killall xr; ");
    //    strcat(startcommand, "/usr/sbin/xr --verbose --server tcp:0:8080 " );
    //    printf("\nCurrent states");
    //    printf("\nIP address\tstatus");
    //    while (current_index<NUMBER_OF_VMs) {
    //        if (vm_data_set[current_index].state==ACTIVE) {
    //            strcat(startcommand, "--backend " );
    //            strcat(startcommand, vm_data_set[current_index].ip_address);
    //            strcat(startcommand, ":8080 ");
    //        }
    //        printf("\n%s\t%i", vm_data_set[current_index].ip_address, vm_data_set[current_index].state);
    //        fflush(stdout);
    //        current_index++;
    //    }
    //
    //    strcat(startcommand, "--web-interface 0:8001 > /dev/null 2>&1 &" );
    //    //printf("\nExecuting command \"killall xr &\"... ");
    //    //fflush(stdout);
    //    //getchar();
    //    //system("killall xr");
    //    printf("\nExecuting command \"%s\"... ",startcommand);
    //    fflush(stdout);
    //    //getchar();
    //    system(startcommand);
}

/* This function compacts groups in vm_data_set 
 * when there are closed connections
 * Using mutex we're sure that there are no gaps
 * in the sequence representation of VMs
 */
void manage_disconnected_vms(struct vm_data * vm){
    int index;
    int group;
    pthread_mutex_lock(&mutex);
    group = vm->service_info.service;
    for (index = 0; index < current_vms[group]; index++) {
        if (strcmp(vm_data_set[group][index].ip_address, vm->ip_address) == 0) {
            vm_data_set[group][index] = vm_data_set[group][--current_vms[group]];
        }
        break;
    }
    pthread_mutex_unlock(&mutex);
}

/* This function looks for a standby VM and 
 * attempts to active it to replace a VM 
 * in rejuvenation state
 */
void switch_active_machine(struct vm_data * vm) {
    int index;
    char send_buff[BUFSIZE];
    
    pthread_mutex_lock(&mutex);
    vm->service_info.state = REJUVENATING;
    for (index = 0; index < current_vms[vm->service_info.service]; index++) {
        if (vm_data_set[vm->service_info.service][index].service_info.state == STAND_BY) {
            vm_data_set[vm->service_info.service][index].service_info.state = ACTIVE;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    vm->last_system_features_stored = 0;
    /*** ASK: notify load balancer with "index" value here? ***/
    
    bzero(send_buff, BUFSIZE);
    send_buff[0] = REJUVENATE;
    if ((send(vm->socket, send_buff, BUFSIZE, 0)) == -1) {
        perror("switch_active_machine - send");
        printf("Closing connection with VM %s\n", vm->ip_address);
        manage_disconnected_vms(vm);
        close(vm->socket);
    } else{
        printf("REJUVENATE command sent to machine with IP address %s\n", vm->ip_address);
        fflush(stdout);
    }
    
}

/* THREAD FUNCTION
 * The duties of this thread are ONLY to listen for messages
 * from a connected VM. If something goes wrong this thread
 * should put a request for its own removal in the incoming
 * requests queue.
 */
void * communication_thread(void * v){
    struct vm_data * vm;
    vm = (struct vm_data *)v;   // this is exactly the struct stored in the system matrix
    int numbytes;
    system_features current_features;
    char recv_buff[BUFSIZE];
    char send_buff[BUFSIZE];

    while (1){
        if (vm->service_info.state == ACTIVE) {
            printf("Waiting for features from the VM %s for the service: %d\n", vm->ip_address, vm->service_info.service);
            bzero(recv_buff,BUFSIZE);
            // check recv features
            if ((numbytes = recv(vm->socket,recv_buff,BUFSIZE,0)) == -1) {
                printf("Failed receiving data from the VM %s for the service: %d\n", vm->ip_address, vm->service_info.service);
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("Timeout on recv() while waiting data from VM %s\n", vm->ip_address);
                }
                break;
            } else  if (numbytes == 0) {
                printf("VM %s is disconnected\n", vm->ip_address);
                break;
            } else{
                // features correctly received
                printf("Received data (%d bytes) from VM %s for the service %d\n", numbytes, vm->ip_address, vm->service_info.service);
                printf("%s\n", recv_buff);
                fflush(stdout);
                get_feature(recv_buff, current_features);
                // at least 2 set of features needed
                if (vm->last_system_features_stored) {
                    float predicted_time_to_crash = get_predicted_rttc(ml_model, vm->last_features, current_features);
                    printf("Predicted time to crash for VM %s for the service %d is: %f\n", vm->ip_address, vm->service_info.service, predicted_time_to_crash);
                    if (predicted_time_to_crash < (float)TTC_THRESHOLD) {
                        /*** ASK: vedere bene queste funzioni e integrarle con il nuovo load balancer ***/
                        switch_active_machine(vm);
                        configure_load_balancer();
                        continue;
                    }
                }
                store_last_system_features(&(vm->last_features),current_features);
                vm->last_system_features_stored = 1;
                //sending CONTINUE command to the VM
                bzero(send_buff, BUFSIZE);
                send_buff[0] = CONTINUE;
                if ((send(vm->socket, send_buff, BUFSIZE,0)) == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        printf("Timeout on send() while sending data by VM %s\n", vm->ip_address);
                        fflush(stdout);
                    } else {
                        printf("Error on send() while sending data by VM %s\n", vm->ip_address);
                        fflush(stdout);
                    }
                    break;
                }
            }
        }
    }
    printf("Closing connection with VM %s\n", vm->ip_address);
    manage_disconnected_vms(vm);
    if(close(vm->socket) == 0){
	printf("Connection correctely closed with VM %s\n", vm->ip_address);
    }
}

/*
 * Listens for incoming connection requests. When such arrives this void
 * fires a new thread to handle it.
 * sockfd - the socketd address of the server
 * pthread_custom_attr - standart attributes for a thread
 */
void accept_new_client(int sockfd, pthread_attr_t pthread_custom_attr){
    unsigned int addr_len;
    int socket;
    int numbytes;
    pthread_t tid;
    int index;
    char * buffer;
    struct vm_service s;
    
    struct sockaddr_in client;
    addr_len = sizeof(struct sockaddr_in);
    buffer = malloc(BUFSIZE);
    
    // get the first pending VM connection request
    printf("Accepting client\n");
    if ((socket = accept(sockfd, (struct sockaddr *) &client, &addr_len)) == -1) {
        perror("accept_new_client - accept");
    }
    
    /*** ASK: should we set with different timeout for "handshake" phase? ***/
    // set socket timeout
    if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    
    // wait for info about service provided by the VM
    // if errors occur close the socket
    printf("Waiting for info by client\n");
    if ((numbytes = recv(socket,&s, sizeof(struct vm_service),0)) == -1) {
        perror("accept_new_client - recv");
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            perror("accept_new_client - recv timeout");
            close(socket);
        }
    } else if(numbytes == 0){
        perror("accept_new_client - disconnected VM");
        close(socket);
    } else{
		
		while(1){
			bzero(buffer,BUFSIZE);
			if(s.service >= NUMBER_GROUPS){
				sprintf(buffer,"groups");
			}
			else sprintf(buffer,"ok");
		
			if ((send(socket, buffer, BUFSIZE,0)) == -1){
				perror("accept_new_client - send");
				if (errno == EWOULDBLOCK || errno == EAGAIN) {
					perror("accept_new_client - send timeout");
					fflush(stdout);
				}
				close(socket);
			}
			if (!strcmp(buffer,"ok")){
				break;
			}
			if ((numbytes = recv(socket,&s, sizeof(struct vm_service),0)) == -1) {
			perror("accept_new_client - recv");
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				perror("accept_new_client - recv timeout");
				close(socket);
			}
			} else if(numbytes == 0){
				perror("accept_new_client - disconnected VM");
				close(socket);
			}
		}
        // It must guarantee that current_vms[s.service] is always incremented
        // I'm not interested if someone reads just before me, i'm interested in avoiding the index overlapping problem
        
        // increment number of connected VMs
        pthread_mutex_lock(&mutex);
        /*** ASK: is it better a loop? ***/
        // while (current_vms[s.service] == allocated_vms[s.service]) {
        // allocate more slots if the memory is not enough
        if (current_vms[s.service] == allocated_vms[s.service]) {
            vm_data_set[s.service] = realloc(vm_data_set[s.service],sizeof(struct vm_data)*2*allocated_vms[s.service]);
            allocated_vms[s.service] *= 2;
        }
        // assignment phase
        vm_data_set[s.service][current_vms[s.service] + 1].service_info = s;
        strcpy(vm_data_set[s.service][current_vms[s.service] + 1].ip_address, inet_ntoa(client.sin_addr));
        vm_data_set[s.service][current_vms[s.service] + 1].socket = socket;
        /*** ASK: is it better to move below index just before the assignment phase? ***/
        // putted here just because of the possibility of future controls on above fields
        // update when we are sure that all the fields are correctely stored
        index = ++current_vms[s.service];
        
        pthread_mutex_unlock(&mutex);
        
        // make a new thread for each VMs
        printf("Host with IP address %s added in group %d\n", vm_data_set[s.service][index].ip_address, s.service);
        pthread_attr_init(&pthread_custom_attr);
        pthread_create(&tid,&pthread_custom_attr,communication_thread,(void *)&vm_data_set[s.service][index]);
    }
    
}

/*
 * This function opens the VMCI server for connections;
 * sockfd_addr - the place at which the server socket
 * address is written.
 */
void start_server(int * sockfd, int port){
    int sock;
    struct sockaddr_in temp;
    
    //Create socket
    if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("start_server - socket");
        exit(1);
    }
    //Server address
    temp.sin_family = AF_INET;
    temp.sin_addr.s_addr = INADDR_ANY;
    temp.sin_port = htons(port);
    //Bind
    if (bind(sock, (struct sockaddr*) &temp, sizeof(temp))) {
        perror("start_server - bind");
        exit(1);
    }
    //Listen for max CONN_BACKLOG
    if (listen(sock, CONN_BACKLOG)) {
        perror("start_server - listen");
        exit(1);
    }
    *sockfd = sock;
    printf("Server started\n");
    
}


int main(int argc,char ** argv){
    int sockfd;
    int port;
    int index;
    pthread_attr_t pthread_custom_attr;
    
    communication_timeout.tv_sec = COMMUNICATION_TIMEOUT;
    communication_timeout.tv_usec = 0;
    //memset(current_index,0,sizeof(int)*NUMBER_VMs);
    
    if (argc != 3) {
        /*** TODO: added argv[0] to avoid warning caused by %s ***/
        printf("Usage: %s tcp_port_number ml_model_number\n", argv[0]);
        exit(1);
    }
    
    port = atoi(argv[1]);
    ml_model = atoi(argv[2]);
    
    //Allocating initial memory for the VMs
    /*** ATTENZIONE : Problema con l'allocazione della malloc, mi permette di scrivere in altre zone di memoria di almeno 4B ***/
    for (index = 0; index < NUMBER_GROUPS; index++) {
        current_vms[index] = 0; // actually no VMs are connected yet
        vm_data_set[index] = malloc(sizeof(struct vm_data)*NUMBER_VMs); // allocate the same memory initially for each groups
        allocated_vms[index] = NUMBER_VMs;  // number of slots allocated for each group
    }
    
    //Open the local connection
    start_server(&sockfd,port);
    
    //Accept new clients
    while(1){
        accept_new_client(sockfd,pthread_custom_attr);
    }
    
    return 0;
}

