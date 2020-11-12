#ifndef FRONTEND
#define FRONTEND

#define BUFSIZE 1024

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "RPC.h"
#include "a1_lib.h"

void call_backend(rpc_t *r, char *user_input);

#endif