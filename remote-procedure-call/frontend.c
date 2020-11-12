#include "frontend.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "RPC.h"
#include "a1_lib.h"

void call_backend(rpc_t *backend, char *user_input) {
  //parse_input
  //if no arg entered, default to 0
  char * cmd = strtok(user_input, " ");
  if (cmd[strlen(cmd) - 1] == '\n') cmd[strlen(cmd) - 1] = 0;
  if (cmd) {
    char * arg1 = strtok(NULL, " ");
    if (arg1) {
      char * arg2 = strtok(NULL, " ");
      if (arg2) {
        RPC_CallTwoArgs(backend, cmd, arg1, arg2);
      } else {
          RPC_CallOneArg(backend, cmd, arg1);
      }
    } else {
        RPC_CallNoArgs(backend, cmd);
      }
    }
}

int main(int argc, char * argv[]) {

  if (argc != 3) {
    printf("Incorrect number of arguments supplied. Please try again and provide the host and port number.\n");
    return 1;
  }

  char * host = argv[1];
  int port = atoi(argv[2]);

  int sockfd;
  char user_input[BUFSIZE] = { 0 };
  char server_msg[BUFSIZE] = { 0 };
  char connection_response[BUFSIZE] = { 0 };

  rpc_t * backend = (rpc_t * ) malloc(sizeof(rpc_t));
  backend = RPC_Connect(host, port);
  //any connection issues will result in a null backend
  if(backend == NULL) {
    printf("Problems connecting to the backend program. Please check your inputs and try restarting the frontend or backend.\n");
    return 0;
  }
  //receive initial message to see if a new connection is allowed or not
  recv_message(backend -> sockfd, connection_response, sizeof(connection_response));
  //if a new connection is not allowed
  if(!strcmp(connection_response, "closed")) {
    printf("Server is no longer accepting connections.\n");
    return 0;
  }

  while (strcmp(backend->host, "") || strcmp(user_input, "quit\n") || strcmp(user_input, "exit\n")) {
    memset(user_input, 0, sizeof(user_input));
    memset(server_msg, 0, sizeof(server_msg));
    printf(">> ");

    // read user input from command line
    fgets(user_input, BUFSIZE, stdin);

    // parse user input and send the command and arguments to server
    call_backend(backend, user_input);
    // receive a message from the server in response to command and args sent
    ssize_t byte_count = recv_message(backend -> sockfd, server_msg, sizeof(server_msg));
    if (byte_count <= 0) {
      break;
    }
    printf("%s\n", server_msg);
    if (strstr(server_msg, "exit") || strstr(server_msg, "quit")) {
      break;
    }
  }

  return 0;
}