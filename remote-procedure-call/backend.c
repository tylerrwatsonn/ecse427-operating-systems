#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "RPC.h"
#include "a1_lib.h"
#include "backend.h"

int addInts(int a, int b) {
  return (a + b);
}

int multiplyInts(int a, int b) {
  return (a * b);
}

float divideFloats(float a, float b) {
  if (b == 0.0) {
    return 0.0;
  }
  return ((float) a / (float) b);
}

uint64_t factorial(int x) {
  uint64_t result = 1;
  while (x > 1) {
    result = result * x;
    x -= 1;
  }
  return result;
}

int executeCommand(char* cmd, float arg1, float arg2, char *response_msg){ 
  if (!strcmp(cmd, "exit")) {
      sprintf(response_msg, "Received exit command! \nCIAO!");
      return 1;
  } else if (!strcmp(cmd, "quit")) {
      sprintf(response_msg, "Received quit command! \nCIAO! I'll handle the shutdown!");
  } else if (!strcmp(cmd, "add")) {
      int result = addInts((int) arg1, (int) arg2);
      sprintf(response_msg, "%d", result);
  } else if (!strcmp(cmd, "multiply")) {
      int result = multiplyInts((int) arg1, (int) arg2);
      sprintf(response_msg, "%d", result);
  } else if (!strcmp(cmd, "divide")) {
      if (arg2 != 0.0) {
        float result = divideFloats(arg1, arg2);
        sprintf(response_msg, "%.5f", result);
      } else {
          sprintf(response_msg, "Error: Division by zero");
      }
  } else if (!strcmp(cmd, "sleep")) {
      sleep((int) arg1);
      sprintf(response_msg, "Slept for %d seconds", (int) arg1);
  } else if (!strcmp(cmd, "factorial")) {
      uint64_t result = factorial((int) arg1);
      sprintf(response_msg, "%lu", result);
  } else {
      sprintf(response_msg, "Error: Command %s not found", cmd);
  }

  return 0; //no exit command
}

int main(int argc, char * argv[]) {

  if (argc != 3) {
    printf("Incorrect number of arguments supplied. Please try again and provide the host and port number.\n");
    return 1;
  }
  char * host = argv[1];
  int port = atoi(argv[2]);

  rpc_t * r = (rpc_t * ) malloc(sizeof(rpc_t));
  r = RPC_Init(host, port);
  if(r == NULL) {
    printf("Problems initializing the backend server. Please investigate the problem and try restarting the backend.\n");
    return 0;
  }
  message_t * msg = (message_t * ) malloc(sizeof(message_t));
  strcpy(msg -> cmd, "");

  int pid;
  int childPids[MAX_CONNECTIONS];
  int frontendFds[MAX_CONNECTIONS];
  int rval = (int *) malloc(sizeof(int));
  int connections = 0;
  int deadChildren;
  bool noMoreConnections = false;
  bool childStillRunning = false;
  while (1) {
    printf("\nGive me a new connection!!!\n");
    //wait for connection
    //design decision that even after a quit command, backend profram runs to refuse incoming connection s
    //and only terminates when all children processes have ended after a quit command and new connection comes in
    //refusing vs accepting a connection will be communicated to frontend through message passing
    if (accept_connection(r -> sockfd, & (r -> clientfd)) == 0) {
      //check flag if shutdown or 5 connections
      if(noMoreConnections || connections == 5) {
        send_message(r->clientfd, "closed", sizeof("closed"));
      } else {
        send_message(r->clientfd, "open", sizeof("open"));

        //Create child process for new connection
        if ((pid = fork()) == 0) {
          printf("child PID: %d", getpid());
    
        while (strcmp(msg -> cmd, "quit")) {
          //handle memory clearing
          char response_msg[50];
          memset(msg, 0, sizeof(msg));
          memset(response_msg, 0, sizeof(response_msg));
          ssize_t byte_count = recv(r -> clientfd, msg, sizeof(message_t), 0);
          if (byte_count <= 0) {
            sprintf(response_msg, "Something went wrong, please try again!");
          } else {
            int exitStatus = executeCommand(msg -> cmd, msg -> args[0], msg -> args[1], response_msg); //returns 1 for exit
            if(exitStatus){
              send_message(r -> clientfd, response_msg, strlen(response_msg));
              close(r->clientfd);
              exit(2);
            }
          }
          send_message(r -> clientfd, response_msg, strlen(response_msg));
        }
        exit(1);
      } else {
        //parent process comes here
        childPids[connections] = pid;
        frontendFds[connections] = r -> clientfd;
        connections += 1;
      }
      }
        printf("new pid: %d\n", pid);
        printf("Connections: %d\n", connections);
        childStillRunning = false;
        deadChildren = 0;
        for (int i = 0; i < connections; i++) {
          rval = 0;
          waitpid(childPids[i], &rval, WNOHANG);
          printf("Returned Val for pid(%d) is %d\n", childPids[i], WEXITSTATUS(rval));

          if (WEXITSTATUS(rval) == 1) { //indicates a "quit"
            noMoreConnections = true;
            childPids[i] = 0;
            close(frontendFds[i]);
            deadChildren += 1;
          } else if(WEXITSTATUS(rval) == 2){ //indicates a "exit"
            childPids[i] = 0;
            close(frontendFds[i]);
            deadChildren +=1;
          } else {
            childStillRunning = true; //this flag will be used to check if all children have terminated
          }
        }
        if(!childStillRunning) {
          printf("Backend is no longer serving any children so program will terminate.\n");
          return 0;
        }
        //update child Pids array to shift existing children in array and free up spot for next connection
        for (int i = 0; i < connections-1; i++) {
          if(childPids[i] == 0 && childPids[i+1] != 0) {
            int j = i;
            while(j>-1 && childPids[j] == 0) {
              childPids[j] = childPids[j+1];
              frontendFds[j] = frontendFds[j+1];
              childPids[j+1] = 0;
              j -=1;
            }
          }
        }
        printf("number of dead children");
        connections -= deadChildren; //update number of active connections
    } else { //occurs if connection acceptance failed
      char* failedConnectionMessage = "Something went wrong with an incoming connection.";
      printf("%s\n", failedConnectionMessage);
      send_message(r->clientfd, failedConnectionMessage, sizeof(failedConnectionMessage));
    }

  }
}