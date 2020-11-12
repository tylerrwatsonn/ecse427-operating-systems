#include <ucontext.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include "sut.h"
#include "queue.h"

#define TOKEN "***"
#define BUFSIZE 2048

//contexts for two kernel threads
ucontext_t c_exec_cntxt;

//kernel threads to run tasks
pthread_t c_exec, i_exec;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//queues
struct queue ready_queue, wait_queue, from_io_queue, to_io_queue;

int numthreads, sockfd;

//booleans to track when the program should end
bool readyQueueEmpty, waitQueueEmpty, firstTaskExecuted = false;


void *c_executor() {

    struct queue_entry *next_task;

    while(true) {
        
        //peek to see if there is a task in the queue
        pthread_mutex_lock(&mutex);
        next_task = queue_peek_front(&ready_queue);
        pthread_mutex_unlock(&mutex);

        if(next_task) {
            readyQueueEmpty = false;

            pthread_mutex_lock(&mutex);
            queue_pop_head(&ready_queue);
            pthread_mutex_unlock(&mutex);

            ucontext_t context = *(ucontext_t *)next_task->data;

            //run task
            swapcontext(&c_exec_cntxt, &context);

            //boolean to ensure that program doesn't end when initial queue is empty
            firstTaskExecuted = true;
        } else if(firstTaskExecuted){
            //check to terminate program
            readyQueueEmpty = true;
            if(waitQueueEmpty) return 0;
        }

        usleep(100);
    }
}

void *i_executor() {
    while(true) {

        //peek to see if there is a task in the queue
        pthread_mutex_lock(&mutex);
        struct queue_entry *to_io_req = queue_peek_front(&to_io_queue);
        pthread_mutex_unlock(&mutex);

        if (to_io_req) {
            waitQueueEmpty = false;

            pthread_mutex_lock(&mutex);
            queue_pop_head(&to_io_queue); 
            pthread_mutex_unlock(&mutex);

            char *request = (char *) to_io_req->data;
            char *cmd = strtok(request, TOKEN);

            if (strcmp(cmd, "open") == 0) {
                char *dest = strtok(NULL, TOKEN);
                int port = atoi(strtok(NULL, TOKEN));

                struct sockaddr_in server_address = { 0 };

                // create a new socket
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                perror("Failed to create a new socket\n");
                    return NULL;
                }

                // connect to server
                server_address.sin_family = AF_INET;
                inet_pton(AF_INET, dest, &(server_address.sin_addr.s_addr));
                server_address.sin_port = htons(port);
                if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                    perror("Failed to connect to server\n");
                    return NULL;
                }

                printf("Opened connection to %s:%d\n", dest, port);

            } else if (strcmp(cmd, "write") == 0) {
                char *msg = strtok(NULL, TOKEN);
                int size = atoi(strtok(NULL, TOKEN));
                //send message
                send(sockfd, msg, size, 0);
                printf("Sent message %s\n", msg);

            } else if (strcmp(cmd, "read") == 0) {
                printf("Read message\n");
                char server_msg[BUFSIZE];
                recv(sockfd, server_msg, sizeof(server_msg), 0);

                struct queue_entry *response = queue_new_node(server_msg);

                //place message in from io queue to be read by task
                pthread_mutex_lock(&mutex);
                queue_insert_tail(&from_io_queue, response);

                //move task from wait queue to ready queue
                struct queue_entry *task_ready = queue_peek_front(&wait_queue);
                if(task_ready) {
                    queue_insert_tail(&ready_queue, task_ready);
                    queue_pop_head(&wait_queue);
                }
                pthread_mutex_unlock(&mutex);


            } else if (strcmp(cmd, "close") == 0) {
                printf("Closing connection\n");
                close(sockfd);
            }
        } else if (firstTaskExecuted) {
            //check if program should be terminated
            waitQueueEmpty = true;
            if(readyQueueEmpty) return 0;
        }
        
        usleep(100 * 1000);
    }
}

void sut_init() {

    //initialize queues
    ready_queue = queue_create();
    queue_init(&ready_queue);

    wait_queue = queue_create();
    queue_init(&wait_queue);

    from_io_queue = queue_create();
    queue_init(&from_io_queue);

    to_io_queue = queue_create();
    queue_init(&to_io_queue);

    //initialize kernel threads
    pthread_create(&c_exec, NULL, c_executor, NULL);
    pthread_create(&i_exec, NULL, i_executor, NULL);
}

bool sut_create(sut_task_f fn) {
    threaddesc *tdescptr = (threaddesc *)malloc(sizeof(threaddesc));
	getcontext(&(tdescptr->threadcontext)); 

    //initialize thread details as in YAUThreads
	tdescptr->threadid = numthreads;
	tdescptr->threadstack = (char *)malloc(THREAD_STACK_SIZE);
	tdescptr->threadcontext.uc_stack.ss_sp = tdescptr->threadstack;
	tdescptr->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
	tdescptr->threadcontext.uc_link = &c_exec_cntxt;
	tdescptr->threadcontext.uc_stack.ss_flags = 0;
	tdescptr->threadfunc = fn;

	makecontext(&(tdescptr->threadcontext), fn, 0);

	numthreads++;

    //add task to ready queue
    struct queue_entry *task = queue_new_node(&(tdescptr->threadcontext));

    pthread_mutex_lock(&mutex);
    queue_insert_tail(&ready_queue, task);
    pthread_mutex_unlock(&mutex);

	return true;
}

void sut_yield() {
    ucontext_t curr_context;
    getcontext(&curr_context);

    //add task to the back of the ready queue
    struct queue_entry *task = queue_new_node(&curr_context);

    pthread_mutex_lock(&mutex);
    queue_insert_tail(&ready_queue, task);
    pthread_mutex_unlock(&mutex);

    swapcontext(&curr_context, &c_exec_cntxt);
    
}

void sut_exit() {
    ucontext_t curr_context;
    getcontext(&curr_context);
    swapcontext(&curr_context, &c_exec_cntxt);
    //don't add task back to the ready queue
}

void sut_open(char *dest, int port) {
    printf("OPEN REQUESTED\n");
    char open_tmp_msg[BUFSIZE];

    sprintf(open_tmp_msg, "open%s%s%s%d", TOKEN, dest, TOKEN, port);

    //to avoid unexpected behaviour in i_exec function when getting string from queue
    char *open_req_msg = strdup(open_tmp_msg);
    
    struct queue_entry *request = queue_new_node(open_req_msg);
    
    pthread_mutex_lock(&mutex);
    queue_insert_tail(&to_io_queue, request);
    pthread_mutex_unlock(&mutex);
}

void sut_write(char *buf, int size) {
    printf("WRITE REQUESTED\n");

    char write_tmp_msg[BUFSIZE];

    sprintf(write_tmp_msg, "write%s%s%s%d", TOKEN, buf, TOKEN, size);

    //to avoid unexpected behaviour in i_exec function when getting string from queue
    char *write_req_msg = strdup(write_tmp_msg);

    struct queue_entry *request = queue_new_node(write_req_msg);
    
    pthread_mutex_lock(&mutex);
    queue_insert_tail(&to_io_queue, request);
    pthread_mutex_unlock(&mutex);

}
void sut_close() {
    char *close_req_msg = "close";
    struct queue_entry *request = queue_new_node(close_req_msg);

    pthread_mutex_lock(&mutex);
    queue_insert_tail(&to_io_queue, request);
    pthread_mutex_unlock(&mutex);
}

char *sut_read() {
    //send request
    ucontext_t curr_context;
    getcontext(&curr_context);

    char *read_req_msg = "read";
    
    struct queue_entry *request = queue_new_node(read_req_msg);
    
    pthread_mutex_lock(&mutex);
    queue_insert_tail(&to_io_queue, request);
    printf("inserted in io_queue\n");
    pthread_mutex_unlock(&mutex);

    //insert task into wait queue
    struct queue_entry *task = queue_new_node(&curr_context);

    pthread_mutex_lock(&mutex);
    queue_insert_tail(&wait_queue, task);
    pthread_mutex_unlock(&mutex);

    swapcontext(&curr_context, &c_exec_cntxt);

    //this will occur once io has read from the server and task is put back in ready queue

    pthread_mutex_lock(&mutex);
    struct queue_entry *from_io_req = queue_peek_front(&from_io_queue);
    if (from_io_req) {
        char *response = (char *) from_io_req->data;
        queue_pop_head(&from_io_queue);
        pthread_mutex_unlock(&mutex);
        //send messsage back to task
        return response;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;

}

void sut_shutdown() {

    //wait on kernel threads to copmlete
    pthread_join(c_exec, NULL);
    pthread_join(i_exec, NULL);

    printf("SUT library execution has ended.\n");
}