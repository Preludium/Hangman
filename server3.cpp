#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <unistd.h>
#include <cstdlib>
#include <sys/epoll.h>

#define MAX_CLIENTS 2
#define MAX_BUFFER 16
using namespace std;

const int one = 1;
vector<int> connectedClients;

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == -1) {
        perror("Server socket error");
        exit(EXIT_FAILURE);
    }

    printf("Starting server on socket %d\n", serverSocket);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("Bind socket error");
        exit(EXIT_FAILURE);
    }

    printf("Listening on 127.0.0.1, port 8080\n");
    listen(serverSocket, 1);

    while (connectedClients.size() < MAX_CLIENTS) {
        int sock = accept(serverSocket, nullptr, nullptr);

        if (sock == -1) {
            perror("Socket error");
            exit(EXIT_FAILURE);
        }
        
        connectedClients.push_back(sock);
        printf("New client accepted on socket %d\n", sock);
        write(sock, "ACCEPT\n", 8);
    }

    int fd = epoll_create1(0);
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serverSocket;

    for (int i=0; i<MAX_CLIENTS; ++i)
        epoll_ctl(fd, EPOLL_CTL_ADD, connectedClients.at(i), &event);

    char buffer[MAX_BUFFER];

    printf("All clients accepted, polling...\n");
    while (true) {
        int ready = epoll_wait(fd, &event, 1, 0);

        if (ready == -1) {
            perror("Poll error");
            exit(1);
        }

        else if (ready > 0) {
            for (int i=0; i< connectedClients.size(); ++i) {
                if (event.events & EPOLLIN && event.data.fd == connectedClients.at(i)) {
                    int len = read(event.data.fd, buffer, MAX_BUFFER);
                    write(1, buffer, len);
                }
            }
        }
    }

    return 0;
}