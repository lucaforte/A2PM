#include <stdio.h>
#include <stdlib.h>
#include "sockhelp.h"
#include "thread.h"
#include "broadcast.h"

#define GLOBAL_CONTROLLER_PORT 4567


static int controllers_sockets[MAX_CONTROLLERS];
static int last_controller_socket = 0;

static callback_struct callbacks[MAX_CALLBACKS];
static int last_callback = 0;


static __thread unsigned char bcast_buff[MAX_MESSAGE];
static __thread bool new_bcast_message = false;



static void do_bcast(void) {

	// loop su tutte le socket in controllers_sockets
}


static void *broadcast_loop(void *args) {
	(void)args;

	// Come forwarder. In più uno switch case sul tipo di messaggio ricevuto

	// Quando mi sveglio dal timeout di select, controllo se new_bcast_message == true, in quel caso mando a tutti il messaggio
	// chiamando do_bcast();
}

void initialize_broadcast(const char *controllers_path) {
	FILE *f;
	char line[128];
	int i = 0;

	f = fopen(controllers_path, "r");
	if(f == NULL) {
		perror("Unable to load the list of controllers");
		exit(EXIT_FAILURE);
	}

	while (fgets(line, 128, f) != NULL) {
		controllers_sockets[i++] = make_connection(GLOBAL_CONTROLLER_PORT, SOCK_DGRAM, line);
	}
	last_controller_socket = i;

	create_thread(broadcast_loop, NULL);

	// Registrare qui un set di function pointer associati ad un tipo di messaggio, come da callback_struct
}


void broadcast(int type, void *payload, size_t size) {


	if(size > MAX_MESSAGE) {
		fprintf(stderr, "%s:%d Sending a message too large\n", __FILE__, __LINE__);
		abort();
	}

	memcpy(bcast_buff, payload, size);

	new_bcast_message = true;
}

void register_callback(int type, void (*f)(int sock, void *content, size_t size)) {
	callbacks[last_callback].type = type;
	callbacks[last_callback++].callback = f;
}
