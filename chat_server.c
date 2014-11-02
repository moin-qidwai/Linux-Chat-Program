#include "chat.h"
#include "chat_server.h"
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>

/* Objective: use the client info struct given to setup client thread send function properly. Also setup exit and depart. Then clean solution and test. */

static char banner[] =
"\n\n\
/*****************************************************************/\n\
/*    CSIS0234_COMP3234 Computer and Communication Networks      */\n\
/*    Programming Assignment                                     */\n\
/*    Client/Server Application - Mutli-thread Chat Server       */\n\
/*                                                               */\n\
/*    USAGE:  ./chat_server    [port]                            */\n\
/*            Press <Ctrl + C> to terminate the server           */\n\
/*****************************************************************/\n\
\n\n";

/* 
 * Debug option 
 * In case you do not need debug information, just comment out it.
 */
#define CSIS0234_COMP3234_DEBUG

/* 
 * Use DEBUG_PRINT to print debugging info
 */
#ifdef CSIS0234_COMP3234_DEBUG
#define DEBUG_PRINT(_f, _a...) \
    do { \
        printf("[debug]<%s> " _f "\n", __func__, ## _a); \
    } while (0)
#else
#define DEBUG_PRINT(_f, _a...) do {} while (0)
#endif


struct chat_server  chatserver;
int port = DEFAULT_LISTEN_PORT;
int sd;

void server_init(void);
void server_run(void);

int send_msg_to_client(int sockfd, char *msg, int command)
{
    struct exchg_msg mbuf;
    int msg_len = 0;
    
    memset(&mbuf, 0, sizeof(struct exchg_msg));
    mbuf.instruction = htonl(command);
    if ( (command == CMD_SERVER_JOIN_OK) || (command == CMD_SERVER_CLOSE ) ){
        mbuf.private_data = htonl(-1);
    } else if (command == CMD_SERVER_BROADCAST) {
        memcpy(mbuf.content, msg, strlen(msg));
        msg_len = strlen(msg) + 1;
        msg_len = (msg_len < CONTENT_LENGTH) ? msg_len : CONTENT_LENGTH;
        mbuf.content[msg_len-1] = '\0';
        mbuf.private_data = htonl(msg_len);
    }
    else if (command == CMD_SERVER_FAIL) {
	if(strcmp(msg, "0") == 0)
		mbuf.private_data = htonl(ERR_JOIN_ROOM_FULL);
	else if(strcmp(msg, "1") == 0)
		mbuf.private_data = htonl(ERR_JOIN_DUP_NAME);
	else
		mbuf.private_data = htonl(ERR_UNKNOWN_CMD);
    }
    
    if (send(sockfd, &mbuf, sizeof(mbuf), 0) == -1) {
        perror("Server socket sending error");
        return -1;
    }
    
    return 0;
}

void insert_client(struct chat_client *node)
{
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode = node;
	struct chat_client *tailnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	tailnode = chatserver.room.clientQ.tail;
	if(chatserver.room.clientQ.count == 0)
	{
		tailnode->next = newnode;
		newnode->next = chatserver.room.clientQ.head;
		newnode->next->prev = newnode;
		newnode->prev = chatserver.room.clientQ.tail;
		chatserver.room.clientQ.count++;
	}
	else
	{
		newnode->next = chatserver.room.clientQ.tail->next;
		newnode->prev = chatserver.room.clientQ.tail->next->prev;
		tailnode->next = newnode;
		chatserver.room.clientQ.count++;
	}
}

void delete_node(struct chat_client *node)
{
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode = chatserver.room.clientQ.tail;
	while(newnode->next != chatserver.room.clientQ.head)
	{
		if(newnode->next == node)
		{
			newnode->next = node->next;
			node->next->prev = newnode;
			chatserver.room.clientQ.count--;
			free(node);
			break;
		}
		newnode = newnode->next;
	}
}

void create_node(int sockfd, struct sockaddr_in addr, char * c_name, pthread_t *c_thread)
{
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode->socketfd = sockfd;
	newnode->address = addr;
	strcpy(newnode->client_name, c_name);
	newnode->client_thread = *c_thread;
	insert_client(newnode);
}

struct chat_client *get_node(int sock_fd)
{
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode = chatserver.room.clientQ.tail;
	while(newnode->next != chatserver.room.clientQ.head && newnode->next != NULL)
	{
		newnode = newnode->next;
		if(newnode->socketfd == sock_fd)
		return newnode;
	}
	return NULL;
}

int is_name(char * name){
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode = chatserver.room.clientQ.tail;
	while(newnode->next != chatserver.room.clientQ.head)
	{
		newnode = newnode->next;
		if(strcmp(newnode->client_name,name) == 0)
		return 1;
	}
	return 0;
}



void *broadcast_thread_fn(void *);
void *client_thread_fn(void *);

void shutdown_handler(int);


/*
 * The main server process
 */
int main(int argc, char **argv)
{

    printf("%s\n", banner);
    
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = DEFAULT_LISTEN_PORT;
    }

    printf("Starting chat server ...\n");
    
    // Register "Control + C" signal handler
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

	// Initilize the server
    server_init();
    
    // Run the server
    server_run();

    return 0;
}


/*
 * Initilize the chatserver
 */
void server_init(void)
{
    // TO DO:
    // Initilize all related data structures
    // 1. semaphores, mutex, pointers, etc.
    // 2. create the broadcast_thread
	
	int i;
	
	for(i=0;i<20;i++)
	{
		chatserver.room.chatmsgQ.slots[i] = malloc(sizeof(chatserver.room.chatmsgQ.slots));
	}

	chatserver.room.clientQ.count = 0;
	chatserver.room.chatmsgQ.head = 0;
	chatserver.room.chatmsgQ.tail = 0;
	
	chatserver.room.clientQ.tail = (struct chat_client*) malloc(sizeof(struct chat_client));
	chatserver.room.clientQ.head = (struct chat_client*) malloc(sizeof(struct chat_client));

	sem_init(&(chatserver.room.chatmsgQ.buffer_empty), 0 , 0);
	sem_init(&(chatserver.room.chatmsgQ.buffer_full), 0 , 20);
	sem_init(&(chatserver.room.chatmsgQ.mq_lock), 0 , 1);

	pthread_create(&(chatserver.room.broadcast_thread), NULL, (void *) broadcast_thread_fn, NULL);

	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
	perror("socket");
        exit(1);
	}
	
	chatserver.address.sin_family = AF_INET;         // host byte order
    	chatserver.address.sin_port = htons(port);     // short, network byte order
    	chatserver.address.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address
    	memset(&(chatserver.address.sin_zero), '\0', 8); // zero the rest of the struct
	
	if (bind(sd, (struct sockaddr *)&chatserver.address, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    	}

	if (listen(sd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
	
} 


/*
 * Run the chat server 
 */
void server_run(void)
{
	
	struct exchg_msg mbuf;
    	int instruction;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	struct sockaddr_in their_addr;
	int new_fd;

	

    while (1) {
        // TO DO:
        // Listen for new connections
        // 1. if it is a CMD_CLIENT_JOIN, try to create a new client_thread
        //  1.1) check whether the room is full or not
        //  1.2) check whether the username has been used or not
        // 2. otherwise, return ERR_UNKNOWN_CMD

	if ((new_fd = accept(sd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
        perror("accept");
        exit(1);
    	}

	recv(new_fd, &mbuf, sizeof(mbuf), 0);
        

	instruction = ntohl(mbuf.instruction);
	if (instruction == CMD_CLIENT_JOIN) {
        	if(chatserver.room.clientQ.count >= 20){
			send_msg_to_client(new_fd, "0", CMD_SERVER_FAIL);
		}
		else if(chatserver.room.clientQ.count > 0 && is_name(mbuf.content) == 1) {
			send_msg_to_client(new_fd, "1", CMD_SERVER_FAIL);
		}
		else {
		int *argu = malloc(sizeof(*argu));
		*argu = new_fd;
		pthread_t t_thread;
		if(pthread_create(&t_thread, NULL, (void *) client_thread_fn, argu) != 0)
		send_msg_to_client(new_fd, "2", CMD_SERVER_FAIL);
		else
		{
			send_msg_to_client(new_fd, NULL, CMD_SERVER_JOIN_OK);
			create_node(new_fd, their_addr, mbuf.content, &t_thread);
 			char * welcome = strcat(mbuf.content," Welcome to the chat room");
			sem_wait(&(chatserver.room.chatmsgQ.buffer_full));
			sem_wait(&(chatserver.room.chatmsgQ.mq_lock));
			strcpy(chatserver.room.chatmsgQ.slots[chatserver.room.chatmsgQ.tail],welcome);
			chatserver.room.chatmsgQ.tail++;
			sem_post(&(chatserver.room.chatmsgQ.mq_lock));
			sem_post(&(chatserver.room.chatmsgQ.buffer_empty));
		}
			}

	}
	else
		send_msg_to_client(new_fd, "2", CMD_SERVER_FAIL);
    }
}
 

void *client_thread_fn(void *arg)
{
    // TO DO:
    // Put one message into the bounded buffer "$client_name$ just joins, welcome!"
    
	struct exchg_msg mbuf;
    	int instruction, sockfd = *((int *) arg);
	struct chat_client *newnode = (struct chat_client*) malloc(sizeof(struct chat_client));
	newnode = get_node(sockfd);
	char * cname;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    	while (1) {
        // TO DO
        // Wait for incomming messages from this client
        // 1. if it is CMD_CLIENT_SEND, put the message to the bounded buffer
        // 2. if it is CMD_CLIENT_DEPART: 
        //  2.1) send a message "$client_name$ leaves, goodbye!" to all other clients
        //  2.2) free/destroy the resources allocated to this client
        //  2.3) terminate this thread

	cname = (char *) malloc(sizeof(char));
	strcpy(cname , newnode->client_name);

	if(recv(newnode->socketfd, &mbuf, sizeof(mbuf), 0 ) <= 0)
	{
		delete_node(newnode);
		close(sockfd);
		pthread_detach(pthread_self());	
		pthread_exit(0);
	}

	else
	{
	instruction = ntohl(mbuf.instruction);
	
	if(instruction == CMD_CLIENT_SEND )
	{
		sem_wait(&(chatserver.room.chatmsgQ.buffer_full));
		sem_wait(&(chatserver.room.chatmsgQ.mq_lock));
		char * message = strcat(cname," ");
		message = strcat(message,mbuf.content);
		strcpy(chatserver.room.chatmsgQ.slots[chatserver.room.chatmsgQ.tail],message);
		chatserver.room.chatmsgQ.tail++;
		sem_post(&(chatserver.room.chatmsgQ.mq_lock));
		sem_post(&(chatserver.room.chatmsgQ.buffer_empty));
	}
	else if(instruction == CMD_CLIENT_DEPART )
	{
		sem_wait(&(chatserver.room.chatmsgQ.buffer_full));
		sem_wait(&(chatserver.room.chatmsgQ.mq_lock));
		char * message = strcat(cname," ");
		message = strcat(message,"just leaves the chat room, goodbye!");
		strcpy(chatserver.room.chatmsgQ.slots[chatserver.room.chatmsgQ.tail],message);
		chatserver.room.chatmsgQ.tail = (chatserver.room.chatmsgQ.tail+1)%20;
		delete_node(newnode);
		sem_post(&(chatserver.room.chatmsgQ.mq_lock));
		sem_post(&(chatserver.room.chatmsgQ.buffer_empty));
		close(sockfd);
		pthread_detach(pthread_self());	
		pthread_exit(0);
	}
	}
    }
} 




void *broadcast_thread_fn(void *arg)
{
	struct chat_client *newnode;

	/* enable cancallation and set the thread cancellation 
	   state to asynchronous */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);	

    while (1) {
        // TO DO:
        // Broadcast the messages in the bounded buffer to all clients, one by one
		newnode = chatserver.room.clientQ.tail;
		sem_wait(&(chatserver.room.chatmsgQ.buffer_empty));	//wait for work item in BB
		sem_wait(&(chatserver.room.chatmsgQ.mq_lock));		//has work item(s); wait for lock
		char * msg = chatserver.room.chatmsgQ.slots[chatserver.room.chatmsgQ.head];
		chatserver.room.chatmsgQ.head = (chatserver.room.chatmsgQ.head+1)%20;
		while(newnode->next != chatserver.room.clientQ.head)
		{
			newnode = newnode->next;
			send_msg_to_client(newnode->socketfd, msg, CMD_SERVER_BROADCAST);
		}
		sem_post(&(chatserver.room.chatmsgQ.mq_lock));		//release lock
		sem_post(&(chatserver.room.chatmsgQ.buffer_full));	//indicate a free slot

    }
}


/*
 * Signal handler (when "Ctrl + C" is pressed)
 */
void shutdown_handler(int signum)
{
    // TO DO:
    // Implement server shutdown here
    // 1. send CMD_SERVER_CLOSE message to all clients
    // 2. terminates all threads: broadcast_thread, client_thread(s)
    // 3. free/destroy all dynamically allocated resources: memory, mutex, semaphore, whatever.

	pthread_cancel(chatserver.room.broadcast_thread);
	pthread_join(chatserver.room.broadcast_thread, NULL);

	struct chat_client *newnode;
	if(chatserver.room.clientQ.tail != NULL){
	newnode = chatserver.room.clientQ.tail;
	while(newnode->next != NULL && newnode->next != chatserver.room.clientQ.head)
	{
		newnode = newnode->next;
		send_msg_to_client(newnode->socketfd, NULL, CMD_SERVER_CLOSE);
		pthread_cancel(newnode->client_thread);
		pthread_join(newnode->client_thread, NULL);
	}
	}
    	
	sem_destroy(&(chatserver.room.chatmsgQ.mq_lock));	//release semaphore resources
	sem_destroy(&(chatserver.room.chatmsgQ.buffer_full));
	sem_destroy(&(chatserver.room.chatmsgQ.buffer_empty));

    exit(0);
}
