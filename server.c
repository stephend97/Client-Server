//////////////////////////////////////
//// Stephen Dean
//// server.c
//// Last Modified: 12/5/17
//// args: port number
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 50 //size of buffer
char port[4];//the port users will be connecting to, command line arg

void sigint_func(int n){
    write(1,"\nExiting...\n", 13);
    fflush(NULL);
    exit(1);
}

void sigchld_func(int n){
    waitpid(-1, NULL, WNOHANG);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	if(argc != 2){
	    fprintf(stderr, "Missing port number.\n");
	    exit(1);
	}
	strcpy(port,argv[1]);

	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv,i;
	int pid;
	char buf[MAXDATASIZE];
	char *token;

	signal(SIGINT, sigint_func);
	signal(SIGCHLD, sigchld_func);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}


	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		pid = fork();
		if(pid==0){
                    inet_ntop(their_addr.ss_family,
                            get_in_addr((struct sockaddr *)&their_addr),
                            s, sizeof s);
                    printf("server: got connection from %s.\n", s);

		    recv(new_fd, buf, sizeof(buf), 0);//recieves file name from client

		    //parses out the file name given
		    token = strtok(buf, " ");
		    char *token_array[50];
		    for(i=0; NULL != token; i++){
			token_array[i] = token;
			token = strtok(NULL, " ");
		    }
		    token_array[1][strlen(token_array[1])] = '\0';


		    printf("Downloading file: %s to client.\n",token_array[1]);
		    FILE *f = fopen(token_array[1], "r");

		    if(NULL == f){
			perror("File null.");
			exit(1);
		    }

		    int num_bytes;
		    while(1){
			num_bytes = fread(buf, 1, sizeof(buf), f);

			if(num_bytes > 0){
			    send(new_fd, buf, sizeof(buf), 0);//send to client is file has stuff
			}
			if(feof(f)){
			    send(new_fd, "cmsc257", 7, 0);//sends terminal string to client at end of file
			}
		    }//end of nested while
		    fclose(f);
                    close(new_fd);
		    exit(1);
		}
		else if(pid<0){
		    printf("Fork failed.\n");

		}
		else{
		    //wait(NULL);
		}
	}//end of while

	return 0;
}//end of main

