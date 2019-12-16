#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <ctime>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <algorithm>
#include <string.h>
#include <poll.h> 
#include <signal.h>
#include <error.h>

using namespace std;

#define ACCEPT "ACCEPT\n"
#define REFUSE "REFUSE\n"
#define KICK "KICK\n"
#define GAME "GAME\n"
#define MAX_LEN 8

int clientSocket;

void handler(int) {
    printf("\n");
    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
    exit(0);
}

ssize_t readData(int fd, char * buffer, ssize_t buffsize){
	auto ret = read(fd, buffer, buffsize);
	if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
	return ret;
}

void writeData(int fd, char * buffer, ssize_t count){
	auto ret = write(fd, buffer, count);
	if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
	if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

int main() {
    signal(SIGINT, handler);

    addrinfo *resolved, hints={.ai_flags=0, .ai_family=AF_INET, .ai_socktype=SOCK_STREAM};
	int res = getaddrinfo("127.0.0.1", "8080", &hints, &resolved);
	if(res || !resolved) error(1, 0, "getaddrinfo: %s", gai_strerror(res));
	
	clientSocket = socket(resolved->ai_family, resolved->ai_socktype, 0);
	if(clientSocket == -1) error(1, errno, "socket failed");
	
	res = connect(clientSocket, resolved->ai_addr, resolved->ai_addrlen);
	if(res) {
        printf("Server is down, try again later.\n");
        exit(0);    
    } //error(1, errno, "connect failed");
	
	freeaddrinfo(resolved);

    int fd = epoll_create1(0);

    epoll_event event;
    event.events = POLLIN;
    event.data.fd = clientSocket;

    epoll_ctl(fd, EPOLL_CTL_ADD, clientSocket, &event);
    epoll_ctl(fd, EPOLL_CTL_ADD, 0, &event);

    while(true) {
        int resultCount = epoll_wait(fd, &event, 1, 0);    
        //printf("%d\n", resultCount);   
        if( event.events & EPOLLIN && event.data.fd == clientSocket ){  
            // read from socket, write to stdout
            ssize_t bufsize1 = MAX_LEN, received1;
            char buffer1[bufsize1];
            // printf("PRZYSZLO\n");
            received1 = readData(clientSocket, buffer1, bufsize1);
            writeData(1, buffer1, received1);
        }
        else if(event.events & EPOLLIN && event.data.fd == 0 ) {
            // read from stdin, write to socket
            ssize_t bufsize2 = MAX_LEN, received2;
            char buffer2[bufsize2];
            // printf("NAPISALES\n");
            received2 = readData(0, buffer2, bufsize2);
            writeData(1, (char *)"KLIENT WROTE: \n", 15);
            writeData(clientSocket, buffer2, received2);
        }
    }
    return 0;	
}