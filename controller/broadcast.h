#pragma once

#include <stdlib.h>
#include "sockhelp.h"

#define MAX_CONTROLLERS 32
#define MAX_CALLBACKS 1024
#define MAX_MESSAGE 1024*1024

typedef struct _cb {
	int type;
	void (*f)(void *content, size_t size) callback;
} callback_struct;

void initialize_broadcast(const char *controllers_path);
void broadcast(void *payload, size_t data);
void register_callback(int type, void (*f)(void *content, size_t size));
