#include "sockhelp.h"
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>


#define MAX_NUM_OF_CLIENTS	4096
#define FORWARD_BUFFER_SIZE	1024


int sock; // Listening socket
int connectlist[MAX_NUM_OF_CLIENTS];  // Array of connected sockets
fd_set socks; // Socket file descriptors we want to wake up for, using select()
int highsock; //* Highest #'d file descriptor, needed for select()

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
	FD_SET(sock, &socks);
	
	/* Loops through all the possible connections and adds
		those sockets to the fd_set */
	
	for (listnum = 0; listnum < MAX_NUM_OF_CLIENTS; listnum++) {
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

void deal_with_data(int listnum) {
	char buffer[FORWARD_BUFFER_SIZE];
	char *cur_char;

	if (sock_gets(connectlist[listnum], buffer, FORWARD_BUFFER_SIZE) < 0) {
		// Closed connection
		close(connectlist[listnum]);
		connectlist[listnum] = 0;
	} else {
		// Received some data
		
		/* TODO: FORWARD HERE! */

	}
}

void read_socks() {
	int listnum;

	/* OK, now socks will be set with whatever socket(s)
	   are ready for reading.  Lets first check our
	   "listening" socket, and then check the sockets
	   in connectlist. */

	if (FD_ISSET(sock, &socks)) {
		handle_new_connection();
	}
	
	for (listnum = 0; listnum < MAX_NUM_OF_CLIENTS; listnum++) {
		if (FD_ISSET(connectlist[listnum],&socks))
			deal_with_data(listnum);
	}
}

int main (int argc, char *argv[]) {
	char *ascport;  
	short int port;       
	struct sockaddr_in server_address; 
	int reuse_addr = 1;
	struct timeval timeout;
	int readsocks; // Number of sockets ready for reading

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
	setnonblocking(sock);

	// Get the address information, and bind it to the socket
	ascport = argv[1];
	port = atoport(ascport);
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

	highsock = sock;
	memset((char *) &connectlist, 0, sizeof(connectlist));

	while (1) {
		build_select_list();

		// This can be even set to infinite... we have to decide
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		readsocks = select(highsock+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);

		if (readsocks < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		if (readsocks == 0) {
			printf(".");
			fflush(stdout);
		} else {
			read_socks();
		}
	}
}