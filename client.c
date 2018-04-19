///////////////////////////////////////////
//// Stephen Dean
//// client.c
//// Last modified 12/5/17
//// args: ip address, port number, number
////   of files to download, sleep time
//// notes: randomization doesn't prevent duplicate files
//

#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 50 // max number of bytes we can get at once
char *ip[70]; // ip address, from args
char *port[4]; // the port client will be connecting to, from args
int number_of_files; //specified number of downloads to be done, from args
int sleep_time; //specified sleep time, from args

//get the time to calculate time used
double ftime(void){
    struct tms t;
    times(&t);
    return(t.tms_utime + t.tms_stime) / 100.0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void thread_handler(int tid){
        int sockfd, numbytes;
        char buf[MAXDATASIZE];
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int i,r;
        char s[INET6_ADDRSTRLEN];
        char *files[] = {"file000.txt","file001.txt","file002.txt","file003.txt","file004.txt","file005.txt","file006.txt",
"file007.txt","file008.txt","file009.txt","file010.txt","file011.txt","file012.txt","file013.txt","file014.txt","file015.txt",
"file016.txt","file017.txt","file018.txt","file019.txt","file020.txt","file021.txt","file022.txt","file023.txt","file024.txt",
"file025.txt"};// array of file names to randomly select from

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                exit(1);//return 1;
        }

        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
                        perror("client: socket");
                        continue;
                }
                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                        perror("client: connect");
                        close(sockfd);
                        continue;
                }
                break;
        }

        if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                exit(1);//return 2;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
        printf("client: connecting to %s\n", s);

        freeaddrinfo(servinfo);  //all done with this structure

        char get[50];//array to place the file name and send to the server

        //randomly selects file
        strcpy(get, "get ");
        r = rand() % 25 + 1;
        strcpy(buf, files[r]);//copy the random file name to the buffer
        strcat(get, buf);// copy the file name to send to server in specified format
	printf("Requesting %s\n",files[r]);

	//send the file name to server
        if((send(sockfd,get,sizeof(get),0)) == -1){
            perror("send");
            close(sockfd);
            exit(1);
        }

        FILE *f;
        f = fopen(files[r],"w");

        if(NULL == f){
            perror("File null.");
	    exit(1);
        }

	printf("Starting %s\n", files[r]);

	//recieves 50 byte chunks of the file until it gets terminal string
	while(1){
	    if((numbytes = read(sockfd, buf, MAXDATASIZE)) == -1){
                perror("recv");
                exit(1);
            }
	    if(strstr(buf, "cmsc257") != NULL){
		break;//recieved the terminal string
	    }
	    fwrite(buf, 1, sizeof(buf), f);//writes the 50 byte chunk to file
	}
	printf("Finished %s\n", files[r]);
        fclose(f);
        close(sockfd);
}

int main(int argc, char *argv[])
{
	double start, stop, used;
	int i;
	//pthread_t tid[number_of_files];
	//pthread_attr_t attr;

	if (argc != 5) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	//sets the command line args to global variables
	strcpy(ip,argv[1]);
	strcpy(port,argv[2]);
	number_of_files = atoi(argv[3]);
	sleep_time = atoi(argv[4]);

	pthread_t tid[number_of_files];
	pthread_attr_t attr;

	start = ftime();

	//creates nunmber of threads specified by user
	for(i=0; i < number_of_files; i++){
	    pthread_attr_init(&attr);
	    if(pthread_create(&tid[i], NULL, thread_handler, i) == -1){
		perror("thread");
	    }
	sleep(sleep_time);
	}
	for(i=0; i<number_of_files; i++){

	    pthread_join(tid[i], NULL);
	}

	stop = ftime();
	used = stop - start;
	printf("Elapsed time: %10.2f \n", used);

	return 0;
}

