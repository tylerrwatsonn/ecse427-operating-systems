#include "RPC.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

rpc_t *RPC_Init(char* host, int port){
  rpc_t *rpc = (rpc_t*) malloc(sizeof(rpc_t));
  int sockfd;
  struct sockaddr_in server_address = { 0 };

  // create TCP socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Failed to create a new socket\n");
    return NULL;
  }

  // set options
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Failed to set options for socket\n");
    return NULL;
  }

  // bind to an address
  server_address.sin_family = AF_INET;
  inet_pton(AF_INET, host, &(server_address.sin_addr.s_addr));
  server_address.sin_port = htons(port);
  if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("Failed to bind to an address\n");
    return NULL;
  }
  // start listening
  if (listen(sockfd, BACKLOG_SIZE) < 0) {
    perror("Failed to listen to socket\n");
    return NULL;
  }

  // struct sockaddr_in connection_address = { 0 };
  // socklen_t addrlen = sizeof(connection_address);

  // // wait for a new connection on the server socket and accept it
  // clientfd = accept(sockfd, (struct sockaddr *)&connection_address, &addrlen);
  // if (clientfd < 0) {
  //   perror("Failed to accept client connection\n");
  //   return rpc;
  // }

  rpc->host = host;
  rpc->port = port;
  rpc->sockfd = sockfd;

  return rpc;
}


// void RPC_Register(rpc_t *r, char *name, callback_t fn){

// }

rpc_t *RPC_Connect(char *host, int port){
  rpc_t *rpc = (rpc_t*) malloc(sizeof(rpc_t));
  int sockfd;
  struct sockaddr_in server_address = { 0 };

  // create a new socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Failed to create a new socket\n");
    return NULL;
  }

  // connect to server
  server_address.sin_family = AF_INET;
  inet_pton(AF_INET, host, &(server_address.sin_addr.s_addr));
  server_address.sin_port = htons(port);
  if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("Failed to connect to server\n");
    return NULL;
  }

  rpc->host = host;
  rpc->port = port;
  rpc->sockfd = sockfd;

  return rpc;
}

void RPC_Close(rpc_t *r){
  close(r->sockfd);
}

void RPC_CallNoArgs(rpc_t *r, char *name) {
  message_t *msg = (message_t*) malloc(sizeof(message_t));
  strcpy(msg->cmd, name);
  send(r->sockfd, msg, sizeof(message_t), 0);
}

void RPC_CallOneArg(rpc_t *r, char *name, char *arg) {
  message_t *msg = (message_t*) malloc(sizeof(message_t));
  strcpy(msg->cmd, name);
  msg->args[0] = atof(arg);
  send(r->sockfd, msg, sizeof(message_t), 0);
}

void RPC_CallTwoArgs(rpc_t *r, char *name, char *arg1, char *arg2) {
  message_t *msg = (message_t*) malloc(sizeof(message_t));
  strcpy(msg->cmd, name);
  msg->args[0] = atof(arg1);
  msg->args[1] = atof(arg2);

  send(r->sockfd, msg, sizeof(message_t), 0);
}