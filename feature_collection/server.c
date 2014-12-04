#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 4096
#ifndef BACKLOG
#define BACKLOG 1024
#endif


/* THREAD FUNCTION
 * The duties of this thread are ONLY to listen for messages
 * from a connected VM. If something goes wrong this thread 
 * should put a request for its own removal in the incoming
 * requests queue.
 */
void *communication_thread(void *s){
	int mysocket;
	int ADD_request_sent;
	ADD_request_sent=0;
	FILE *f;
	

	// myID should be renamed as "socket" or something like that...
	mysocket = (int)(long)s;

	printf("Starting to gather data from VM...\n");

	f = fopen("db.txt", "a");
	if(f == NULL) {
		printf("Error opening db.txt\n");
		exit(1);
	}

	fprintf(f, "------------NEW ACTIVE MACHINE:----------\n");
		
	for(;;){
//		sys_features features;
		int numbytes;
//		int newstage;
		char recv_buf[BUFSIZE];
//,send_buf[BUFSIZE];
//		int RESP_TO_GET;
				
		//recv() from socket
		//TODO time-out
		if ((numbytes = recv(mysocket, recv_buf, BUFSIZE, 0)) == -1) {
		      perror("recv");
		      break;
		}
		if(numbytes == 0) {
			fprintf(stderr, "Connection from VM terminated. Waiting for new connection...\n");
			return NULL;
		}
		printf(".");
		fflush(stdout);
		fprintf(f, "%s\n", recv_buf);
		fflush(f);
		bzero(recv_buf, BUFSIZE);
	}
}

/*
  * Listens for incoming connection requests. When such arrives this void 
  * fires a new thread to handle it.
  * vms - the datastructure in which we store the information about the virtual machines
  * sockfd - the socketd address of the server
  * pthread_custom_attr - standart attributes for a thread
  */
void accept_new_client(int sockfd, pthread_attr_t pthread_custom_attr){
	unsigned int addr_len;
	pthread_t tid;
	int socket;

	printf("Accepting ...\n");
	struct sockaddr_in client;
	addr_len = sizeof(struct sockaddr_in);
	if ((socket = accept(sockfd, (struct sockaddr *)&client, &addr_len)) == -1) {
		perror("accept() failed");
	}
	
	printf("Accepting successful\n");

	pthread_create(&tid, NULL, communication_thread, (void *)(long)socket);
}

 
 
/*
 * This function opens the VMCI server for connections;
 * sockfd_addr - the place at which the server socket
 * address is written.
 */
void start_server(int *sockfd_addr){
	int sock,error;
	struct sockaddr_in temp;
	//Create socket
	sock=socket(AF_INET,SOCK_STREAM,0);
	//Server address
	temp.sin_family=AF_INET;
	temp.sin_addr.s_addr=INADDR_ANY;
	temp.sin_port=htons(4444);
	//Bind
	error=bind(sock,(struct sockaddr*) &temp,sizeof(temp));
	error=listen(sock,BACKLOG);

	*sockfd_addr=sock;
}

void terminate(int s) {
	printf("\n");
	exit(0);
}

int main(void) {
	int sockfd;
	pthread_attr_t pthread_custom_attr;
	signal(SIGPIPE, terminate);
	//open the local connection
	start_server(&sockfd);
	//accept new client
	for(;;) {
		accept_new_client(sockfd, pthread_custom_attr);
	}
	
	return 0;
}
