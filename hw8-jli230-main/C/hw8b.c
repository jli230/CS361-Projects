#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/epoll.h>

// struct ClientState {
//     int fd;
//     int message_count;
// };

enum {WAIT_CLIENTS, WAIT_ASKER, WAIT_ANSWERER, EXIT} state;
int askerfd = 0;
int answererfd = 0;
int actives = 0;

typedef struct client{
	int fd;
	enum {ACTIVE = 1, INACTIVE = 2} status;
	pthread_t thread;
	struct client* next;
	enum {ASKER = 1, ANSWERER = 2} type;
} client;

int main(int argc, char const* argv[]) {
    int PORT = 8080;
	if(argc!=2){
		printf("Expected format: ./server <port number>\n");
		exit(0);
	}
	PORT = atoi(argv[1]);
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    int res = bind(sfd,(struct sockaddr*) &addr,sizeof(addr));
    if(res) {
        printf("There was an error binding!\n");
        perror("binding");
        exit(1);
    }

    listen(sfd,10);
    printf("Waiting for connections!\n");

    int epfd = epoll_create1(0);
    struct epoll_event e;
    e.events = EPOLLIN;
    e.data.fd = sfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&e); 
    
    struct epoll_event events[10];    

    int gotevents;
    while((gotevents = epoll_wait(epfd,events,10,-1)) >= 0) {
        printf("I got an event on fd %d!\n",events[0].data.fd);        
        for(int i=0;i<gotevents;i++) {
            if (events[i].data.fd == sfd) {
                printf("Got a new client!\n");
                int cfd = accept(sfd,0,0);
                write(cfd,"Your command: ",14);   

                struct epoll_event e;                
                e.events = EPOLLIN;
                // struct ClientState *s = malloc(sizeof(struct ClientState));
                struct client *s = malloc(sizeof(struct client));
                s->fd = cfd;
                e.data.ptr = s;

                if (actives == 0) {
                    s->type = ASKER;
                    send(s->fd, "Awaiting answerer...\n", strlen("Awaiting answerer...\n"), MSG_CONFIRM);
                    state = WAIT_CLIENTS;
                    askerfd = s->fd;
                } else {
                    s->type = ANSWERER;
                    send(s->fd, "Awaiting question...\n", strlen("Awaiting question...\n"), MSG_CONFIRM);
                    char* message = "You may now ask a question: \n";
                    int msglen = strlen(message);
                    send(askerfd, message, msglen, MSG_CONFIRM);
                    state = WAIT_ASKER;
                    answererfd = s->fd;
                }
                actives++;
                epoll_ctl(epfd, EPOLL_CTL_ADD,cfd,&e);
            }
            else {
                char buf[100];
                // struct ClientState *s = events[i].data.ptr;
                struct client *s = events[i].data.ptr;
                int gotbytes = read(s->fd,buf,100);
                buf[gotbytes]=0;
                // printf("I got message %d: %s\n",s->message_count++,buf);
                
                if (gotbytes <= 0 && state != WAIT_CLIENTS) {
                    perror("Read failed");
                    close(s->fd);
                    s->status = INACTIVE;
                    if(s->fd == answererfd){
                        send(askerfd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
                        state = EXIT;
                        // asker->status = INACTIVE;
                        close(askerfd);
                        // pthread_kill(asker->thread);
                    } else {
                        send(answererfd, "The other party has left the conversation\n", strlen("The other party has left the conversation\n"), MSG_CONFIRM);
                        state = EXIT;
                        // answerer->status = INACTIVE;
                        close(answererfd);
                        // pthread_kill(answerer->thread);
                    }
                    break;
                }
                else {
                    int message_len = strlen(buf);

                    if (s->fd == answererfd) {
                        if (state == WAIT_ASKER) {
                            send(s->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
                            continue;
                        } else {
                            send(askerfd, "Answer: ", strlen("Answer: "), MSG_CONFIRM);
                            send(askerfd, buf, message_len, MSG_CONFIRM);
                            send(askerfd, "You may now ask another question: \n", strlen("You may now ask another question: \n"), MSG_CONFIRM);
                            send(answererfd, "Awaiting question...\n", strlen("Awaiting question...\n"), MSG_CONFIRM);
                            state = WAIT_ASKER;
                        }
                    } else if (s->fd == askerfd) {
                        if (state == WAIT_CLIENTS || state == WAIT_ANSWERER) {
                            send(s->fd, "Patience please\n", strlen("Patience please\n"), MSG_CONFIRM);
                            continue;
                        } else {
                            send(answererfd, "Question: ", strlen("Question: "), MSG_CONFIRM);
                            send(answererfd, buf, message_len, MSG_CONFIRM);
                            state = WAIT_ANSWERER;
                        }
                    }
                    int i = 0;
                    while (buf[i] != '\0') {
                        buf[i] = '\0';
                        i++;
                    }
                }
            }
        }
    }

/*
    while(1) {
//        int cfd = accept(sfd,0,0);

        while(1) {
            write(cfd,"Your command: ",14);   
            char buf[100];
            read(cfd,buf,100);
            printf("I got: %s\n",buf);

            char *line = malloc(100);
            int hundred=100;
            getline(&line,&hundred,stdin);
            printf("Local input: %s\n",line);
            write(cfd,line,strlen(line));
            }
    }*/
}