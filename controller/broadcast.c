#include <stdio.h>
#include <stdlib.h>
#include "sockhelp.h"
#include "thread.h"
#include "broadcast.h"


static int controllers_sockets[MAX_CONTROLLERS];

static callback_struct callbacks[MAX_CALLBACKS];
static int last_callback = 0;

static void *broadcast_loop(void *args) {
	(void)args;

	// Come forwarder. In più uno switch case sul tipo di messaggio ricevuto
}

void initialize_broadcast(const char *controllers_path) {
	FILE* f;
	char line[128];
	int i = 0;

	f = fopen(controllers_path, "r");
	if(f == NULL) {
		perror("Unable to load the list of controllers");
		exit(EXIT_FAILURE);
	}

	while (fgets(line, 128, f) != NULL) {
		controller_sockets[i++] = make_connection(GLOBAL_CONTROLLER_PORT, SOCK_DGRAM, line);
	}

	create_thread(broadcast_loop, NULL);

	// Registrare qui un set di function pointer associati ad un tipo di messaggio, come da callback_struct
}


void broadcast(void *payload, size_t data) {
	// Entrare in spin su un lock locale che dice se la precedente broadcast è andata a buon fine
	// Copiare il contenuto del payload in un buffer globale
	// Notificare il broadcast thread che c'è qualcosa da mandare a tutti
	// ...
}

void register_callback(int type, void (*f)(void *content, size_t size)) {
	callbacks[last_callback].type = type;
	callbacks[last_callback++].callback = f;
}
