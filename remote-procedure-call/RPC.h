 #ifndef RPC_SERV
 #define RPC_SERV

 #define BACKLOG_SIZE 10

//struct to keep track of important info
 typedef struct rpc_t {
  char* host;
  int port;
  int sockfd;
  int clientfd;
} rpc_t;

//struct to send user input from frontend to backend
typedef struct message_t {
  char cmd[15];
  float args[2];
} message_t;

//Initialize backend
rpc_t *RPC_Init(char* host, int port);

//Connect to backend
rpc_t *RPC_Connect(char *name, int port);

//close backend sockfd
void RPC_Close(rpc_t *r);

//send message from frontend to backend
void RPC_CallNoArgs(rpc_t *r, char *name);

void RPC_CallOneArg(rpc_t *r, char *name, char *arg);

void RPC_CallTwoArgs(rpc_t *r, char *name, char *arg1, char *arg2);

 #endif