#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define MAX_SIZE 1460
#define MAX_CLIENTS 2

void insert(int fd);
void remove_inactives();
void *client_handler(void *args);


enum {WAIT_CLIENTS, WAIT_ASKER, WAIT_ANSWERER, EXIT} state;
int askerfd = 0;
int answererfd = 0;

typedef struct client{
	int fd;
	enum {ACTIVE = 1, INACTIVE = 2} status;
	pthread_t thread;
	struct client* next;
	enum {ASKER = 1, ANSWERER = 2} type;
} client;

client* head = NULL;

client * asker;
client * answerer;
int actives = 0;
char *reply = "Server: Message Broadcasted\n";

// insert first
void insert(int fd) {
	client* c = (client*) malloc(sizeof(client));
	c->fd = fd;
	c->next = head;
	c->status = ACTIVE;
	if (actives == 0) {
		c->type = ASKER;
		send(c->fd, "Awaiting answerer...\n", strlen("Awaiting answerer...\n"), MSG_CONFIRM);
		state = WAIT_CLIENTS;
		askerfd = fd;
		asker = c;
	} else {
		c->type = ANSWERER;
		send(c->fd, "Awaiting question...\n", strlen("Awaiting question...\n"), MSG_CONFIRM);
		char* message = "You may now ask a question: \n";
		int msglen = strlen(message);
		send(askerfd, message, msglen, MSG_CONFIRM);
		state = WAIT_ASKER;
		answererfd = fd;
		answerer = c;
	}
	if (pthread_create(&c->thread,NULL,client_handler,(void *)c)) {
		printf("pthread_create failed\n");
		close(c->fd);
		free(c);
		return;
	}
	head = c;
	actives++;
	printf("Currently %d active, %d is added\n", actives, c->fd);
}

void remove_inactives() {
	if (head == NULL) return;
	// if in head
	while (head && head->status == INACTIVE) {
		client* temp = head;
		head = head->next;
		actives--;
		printf("Currently %d active, %d is removed\n", actives, temp->fd);
		free(temp);
	}

	// elsewhere
	client* current = head;
	client* previous = NULL;
	while(current != NULL) {
		if (current->status == INACTIVE) {
			previous->next = current->next;
			actives--;
			printf("Currently %d active, %d is removed\n", actives, current->fd);
			free(current);
			current = previous->next;
		} else {
			previous = current;
			current = current->next;
		}
	}
}


void printList() {
	client* c = head;
	while (c != NULL) {
		printf("%d -> ", c->fd);
		c = c->next;
	}
	printf("\n");
}

/*void broadcast_all(int fd, char *buffer){
	char broadcast_message[MAX_SIZE];
	snprintf(broadcast_message,sizeof(broadcast_message),"%s\n",buffer);
	int message_len = strlen(broadcast_message);
	for(client* c = head;c != NULL; c = c->next){
		if ((c->fd == askerfd && state == WAIT_CLIENTS)) {
			send(c->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
			return;
		}
		if (c->fd == fd) {
			continue;
		} 
		if(c->fd==answererfd){
			if (flag == 1) {
				send(askerfd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
				return;
			}
			send(answererfd, "Question: ", strlen("Question: "), MSG_CONFIRM);
		} else {
			if (flag == -1) {
				send(answererfd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
				return;
			}
			send(askerfd, "Answer: ", strlen("Answer: "), MSG_CONFIRM);
		}
		send(c->fd, broadcast_message, message_len, MSG_CONFIRM);
		if (c->fd == askerfd) {
			send(askerfd, "You may now ask another question\n", strlen("You may now ask another question\n"), MSG_CONFIRM);
		}
		flag = flag * -1;
		// printf("Sending to client %d\n", c->fd);
		// //To-Do 6: Broadcast Message across all clients
		// if ((flag == -1 && c->type == ANSWERER) || (flag == 1 && c->type == ASKER) || flag == 0) {
		// 	send(c->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
		// } else
		// if (send(c->fd, broadcast_message, message_len, MSG_CONFIRM) == -1 ) {
		// 	perror("Broadcast send failed!");
		// 	close(c->fd);
		// 	c->status = INACTIVE;
		// }
	}
}*/

void *client_handler(void *args){
	client *c = (client*) args;
	while(1){
		// if (flag != 0 && actives < 2) {
		// 	send(c->fd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
		// 	close(c->fd);
		// 	c->status = INACTIVE;
		// 	break;
		// }
		char buffer[MAX_SIZE];
		//To-Do 5(a): Read from the client
		// fcntl(c->fd, F_SETFL, O_NONBLOCK);
		int size = read(c->fd, buffer, MAX_SIZE);
		// if (c->fd == answererfd) {
		// 	if(actives < 2 && state != WAIT_CLIENTS){
		// 		send(answererfd, "Connection closed\n", strlen("Connection closed\n"), MSG_CONFIRM);
		// 		return 0;
		// 	}
		// } else if (c->fd == askerfd) {

		// }
		// if (size == 0) {
		// 	send(c->fd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
		// 	close(c->fd);
		// 	c->status = INACTIVE;
		// 	break;
		// }
		if (size <= 0 && state != WAIT_CLIENTS) {
			perror("Read failed");
			close(c->fd);
			c->status = INACTIVE;
			if(c->fd == answererfd){
				send(askerfd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
				state = EXIT;
				asker->status = INACTIVE;
				close(askerfd);
				// pthread_kill(asker->thread);
			} else {
				send(answererfd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
				state = EXIT;
				answerer->status = INACTIVE;
				close(answererfd);
				// pthread_kill(answerer->thread);
			}
			remove_inactives();
			break;
		} else if (buffer[0] != '\0') {
			// broadcast_all(c->fd, buffer);
			//To-Do 5(b): Reply back to client with reply message
			// if (send(c->fd, "Message received\n", strlen("Message received\n"), MSG_CONFIRM) == -1) {
			// 	perror("Send failed");
			// 	close(c->fd);
			// 	c->status = INACTIVE;
			// } else {
			// 	flag = flag * -1;
			// }

			// char broadcast_message[MAX_SIZE];
			// snprintf(broadcast_message,sizeof(broadcast_message),"%s\n",buffer);
			int message_len = strlen(buffer);

			if (c->fd == answererfd) {
				if (state == WAIT_ASKER) {
					send(c->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
					continue;
				} else {
					send(askerfd, "Answer: ", strlen("Answer: "), MSG_CONFIRM);
					send(askerfd, buffer, message_len, MSG_CONFIRM);
					send(askerfd, "You may now ask another question: \n", strlen("You may now ask another question: \n"), MSG_CONFIRM);
					send(answererfd, "Awaiting question...\n", strlen("Awaiting question...\n"), MSG_CONFIRM);
					state = WAIT_ASKER;
				}
			} else if (c->fd == askerfd) {
				if (state == WAIT_CLIENTS || state == WAIT_ANSWERER) {
					send(c->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
					continue;
				} else {
					send(answererfd, "Question: ", strlen("Question: "), MSG_CONFIRM);
					send(answererfd, buffer, message_len, MSG_CONFIRM);
					state = WAIT_ANSWERER;
				}
			}

			int i = 0;
			while (buffer[i] != '\0') {
				buffer[i] = '\0';
				i++;
			}
		}
	}
	remove_inactives();
}

int main(int argc, char const* argv[]){
	
	int PORT = 5000;
	if(argc!=2){
		printf("Expected format: ./server <port number>\n");
		exit(0);
	}
	PORT = atoi(argv[1]);
	//To-Do 1: Create a IPv4 Socket for TCP and store fd in socketfd
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd == -1){
		printf("Could not create Socket\n");
		exit(0);
	}	
	//To-Do 2: Bind to localhost use server_address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	inet_aton("0.0.0.0", &server_address.sin_addr.s_addr);
	if(bind(socketfd, (struct sockaddr *) &server_address, sizeof(server_address))){
		perror("Could not bind the Socket");
		close(socketfd);
		exit(0);
	}
	//To-Do 3: Start listening for clients
	if(listen(socketfd, MAX_CLIENTS)){
		perror("Not able to listen on the Socket");
		close(socketfd);
		exit(0);
	}
	printf("Server running on localhost: %d \n",PORT);
	while(1){
		if(actives == MAX_CLIENTS){
			continue;
		}
		struct sockaddr_in client_address;
		int client_addr_len = sizeof(client_address);
		//To-Do 4: Accept Connections from the clients and store fd associated with connection in connfd
		int connfd = accept(socketfd, (struct sockaddr *) &client_address, (socklen_t *) &client_addr_len);
		
		
		if(connfd < 0){
			printf("Server Could not accept the connection\n");
			continue;
		}	
		insert(connfd);
		printf("Connection Accepted for the Client with fd %d\n",connfd);
		printList();
	}
	return 0;
}
