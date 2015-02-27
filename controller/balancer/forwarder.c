#include "sockhelp.h"
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>

#include "thread.h"


#define MAX_NUM_OF_CLIENTS	1024
#define FORWARD_BUFFER_SIZE	20*1024*1024


int sock; // Listening socket
__thread char *buffer_from_client;
__thread char *buffer_to_client;
__thread int connectlist[2];  // One thread handles only 2 sockets
__thread fd_set socks; // Socket file descriptors we want to wake up for, using select()
__thread int highsock; //* Highest #'d file descriptor, needed for select()

/* TODO: integrare con il resto del codice per sapere qual è la VM */
/**/
int da_socket = -1;

#define TPCW_IP "127.0.0.1"
#define TPCW_PORT "8080"

void setnonblocking(int sock);

void da_socket_is_sigpiped(void) {
	close(da_socket);
	da_socket = -1;
}

int get_current_socket(void) {

	if(da_socket == -1) {
		da_socket = make_connection(TPCW_PORT, SOCK_STREAM, TPCW_IP);
		setnonblocking(da_socket);  // TODO: E' importante fare questa chiamata quando viene passata la socket della nuova VM!
	}

	return da_socket;

}
/**/
/* UP TO HERE */



void setnonblocking(int sock) {
	int opts;

	opts = fcntl(sock, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}
	return;
}

void build_select_list() {
	int listnum; //Current item in connectlist for for loops

	/* First put together fd_set for select(), which will
	   consist of the sock veriable in case a new connection
	   is coming in, plus all the sockets we have already
	   accepted. */
	
	FD_ZERO(&socks);
	
	/* Loops through all the possible connections and adds
		those sockets to the fd_set */

	// Note that one thread handles only 2 sockets, one from the
	// client, the other to the VM!
	
	for (listnum = 0; listnum < 2; listnum++) {
		if (connectlist[listnum] != 0) {
			FD_SET(connectlist[listnum],&socks);
			if (connectlist[listnum] > highsock) {
				highsock = connectlist[listnum];
			}
		}
	}
}


void handle_new_connection(void) {
	int listnum;
	int connection;

	connection = accept(sock, NULL, NULL);
	if (connection < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	setnonblocking(connection);
	
	for (listnum = 0; (listnum < MAX_NUM_OF_CLIENTS) && (connection != -1); listnum ++) {
		if (connectlist[listnum] == 0) {
			printf("\nConnection accepted:   FD=%d; Slot=%d\n",
				connection,listnum);
			connectlist[listnum] = connection;
			connection = -1;
		}
	}

	if (connection != -1) {
		/* No room left in the queue! */
		printf("\nNo room left for new client.\n");
		sock_puts(connection,"Sorry, this server is too busy. Try again later!\n");
		close(connection);
	}
}




void deal_with_data(void *sockptr) {
	char *cur_char;
	int readsocks; // Number of sockets ready for reading
	int vm_socket; // The socket of the current VM
	int client_socket = *(int *)sockptr;
	struct timeval timeout;

	int bytes_ready_from_client = 0;
	int bytes_ready_to_client = 0;

	int transferred_bytes; // How many bytes are transferred by a sock_write or sock_read operation

	buffer_from_client = malloc(FORWARD_BUFFER_SIZE);
	buffer_to_client = malloc(FORWARD_BUFFER_SIZE);

	bzero(buffer_from_client, FORWARD_BUFFER_SIZE);
	bzero(buffer_to_client, FORWARD_BUFFER_SIZE);

	bzero(&connectlist, sizeof(connectlist));


	// Connect to the VM and build the sockets table
	vm_socket = get_current_socket();
	connectlist[0] = client_socket;
	connectlist[1] = vm_socket;


	while (1) {
                build_select_list();

                // This can be even set to infinite... we have to decide
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                readsocks = select(highsock + 1, &socks, &socks, (fd_set *) 0, &timeout);

                if (readsocks < 0) {
                        perror("select");
                        exit(EXIT_FAILURE);
                }
                if (readsocks == 0) {
                        printf("(%d)", tid);
                        fflush(stdout);
                } else {
			
			// Check if the client is ready
			if(FD_ISSET(client_socket, &socks)) {

				// First of all, check if we have something for the client
				if(bytes_ready_to_client > 0) {
					
					printf("Writing to client:\n%s\n---\n", buffer_to_client);

					if((transferred_bytes = sock_write(client_socket, buffer_to_client, bytes_ready_to_client)) < 0) {
						perror("sending to client from buffer");
					}

					bytes_ready_to_client = 0;
					bzero(buffer_to_client, FORWARD_BUFFER_SIZE);
				}

				// Then, see if we have something coming from the client
				// but only if we are not going to overwrite womething
				if(bytes_ready_from_client == 0) {
					if((transferred_bytes = sock_read(client_socket, buffer_from_client, FORWARD_BUFFER_SIZE)) < 0) {
	                                	perror("sending to client from buffer");
	                                }
					bytes_ready_from_client = transferred_bytes;
				}

				if(bytes_ready_from_client > 0) {
					printf("Read from client:\n%s\n---\n", buffer_from_client);
				}
				
			}

			// Check if the VM is ready
			if(FD_ISSET(vm_socket, &socks)) {

				// First of all, check if we have something for the VM to send
                                if(bytes_ready_from_client > 0) {

					printf("Writing to VM:\n%s\n---\n", buffer_from_client);

                                        if((transferred_bytes = sock_write(vm_socket, buffer_from_client, bytes_ready_from_client)) < 0) {
                                                perror("sending to client from buffer");
                                        }

                                        bytes_ready_from_client = 0;
                                        bzero(buffer_from_client, FORWARD_BUFFER_SIZE);
                                }

                                // Then, see if we have something coming from the client
				if(bytes_ready_to_client == 0) {
	                                if((transferred_bytes = sock_read(vm_socket, buffer_to_client, FORWARD_BUFFER_SIZE)) < 0) {
	                                        perror("sending to client from buffer");
	                                }
					bytes_ready_to_client = transferred_bytes;
				}
				
				if(bytes_ready_to_client > 0) {
					printf("Read from VM:\n%s\n---\n", buffer_to_client);
				}

			} 

                }
        }

}



int main (int argc, char *argv[]) {
	char *ascport;  
	short int port;       
	struct sockaddr_in server_address; 
	int reuse_addr = 1;
	struct timeval timeout;
	int readsocks; // Number of sockets ready for reading
	int connection;

	if (argc < 2) {
		printf("Usage: %s PORT\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Avoid TIME_WAIT and set the socket as non blocking
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

	// Get the address information, and bind it to the socket
	ascport = argv[1];
	port = atoport(ascport, NULL);
	memset((char *) &server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = port;
	if (bind(sock, (struct sockaddr *) &server_address,
	  sizeof(server_address)) < 0 ) {
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	listen(sock, MAX_NUM_OF_CLIENTS);

	while(1) {
		connection = accept(sock, NULL, NULL);
	        if (connection < 0) {
	                perror("accept");
	                exit(EXIT_FAILURE);
        	}

		setnonblocking(connection);

		printf("accepted from %d\n", connection);

		create_thread(deal_with_data, &connection);

	}
}
