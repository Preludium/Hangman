#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <ctime>
#include <sys/epoll.h>
#include <poll.h>
#include <signal.h>

#define COUNTDOWN_TIME 5

using namespace std;
const int one = 1;

int serverSocket;
vector<int> waitingClients;
thread newClientsThread, gameThread;
mutex clientsMtx, countdownMtx, handlerMtx;

void handleNewConnections();
void handleGame();

int main(int argc, char** argv) {

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == -1) {
        perror("Socket creation error");
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

    newClientsThread = thread(handleNewConnections);
    gameThread = thread(handleGame);

    newClientsThread.join();
    gameThread.join();

    return 0;
}

void handleNewConnections() {
    while(true) {
        int clientSock = accept(serverSocket, nullptr, nullptr);

        if (clientSock == -1) {
            perror("Client socket creation error");
            continue;
        }

        printf("New client connected on socket %d\n", clientSock);
        write(clientSock, "ACCEPT\n", sizeof("ACCEPT\n"));

        clientsMtx.lock();
        waitingClients.push_back(clientSock);
        clientsMtx.unlock();
    }
} 

void handleGame() {
    while (true) {
        sleep(3);

        printf("Set up pollfd, mutex locked\n");
        clientsMtx.lock();

        int clients = waitingClients.size();
        pollfd ppoll[5]{};
        for (int i=0; i<clients; ++i) {
            ppoll[i].fd = waitingClients.at(i);
            ppoll[i].events = POLLIN;
        }

        clientsMtx.unlock();
        printf("Mutex unlocked, pollfd ready\n");

        char buffer[16];
        clock_t begClk = clock();
        int seconds = 0, elapsed = (clock() - begClk) / CLOCKS_PER_SEC;

        printf("Begin countdown\n");
        while (elapsed < COUNTDOWN_TIME) {
            int ready = poll(ppoll, clients, 0);
            if (ready == -1) {
                perror("Poll error");
                exit(1);
            }

            else if (ready > 0) {
                printf("Ready events: %d\n", ready);
                for (int i=0; i<clients; ++i) {
                    if (ppoll[i].revents & POLLIN) {
                        int len = read(ppoll[i].fd, buffer, 16);
                        write(1, buffer, len);
                    }
                    if (ppoll[i].revents & POLLERR) {
                        printf("POLLERR\n");
                        exit(1);
                    }
                    if (ppoll[i].revents & POLLHUP) {
                        printf("POLLHUP\n");
                        exit(1);
                    }
                    if (ppoll[i].revents & POLLNVAL) {
                        printf("POLLNVAL\n");
                        exit(1);
                    }

                }
            }

            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf("Time elapsed: %d\n", ++seconds);

        }
        printf("End countdown\n");
    }
}