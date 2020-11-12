#ifndef BACKEND
#define BACKEND

#define MAX_CONNECTIONS 5

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "RPC.h"
#include "a1_lib.h"

int addInts(int a, int b);

int multiplyInts(int a, int b);

float divideFloats(float a, float b);

uint64_t factorial(int x);

int executeCommand(char* cmd, float arg1, float arg2, char *response_msg);

#endif