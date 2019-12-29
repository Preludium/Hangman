#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <ctime>
#include <poll.h>
#include <fcntl.h>
#include <algorithm>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define COUNTDOWN_TIME 5

using namespace std;
const int one = 1;

int serverSocket;
vector<int> waitingClients;
thread newClientsThread, gameThread;
mutex clientsMtx, countdownMtx;

void handleNewConnections();
void handleGame();

int main(int argc, char** argv) {

    fcntl(0, F_SETFL, O_NONBLOCK);
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == -1) {
        perror(RED "Socket creation error" RESET);
        exit(EXIT_FAILURE);
    }

    printf("Starting server on socket %d\n", serverSocket);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*) &addr, sizeof(addr)) == -1) {
        perror(RED "Bind socket error" RESET);
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
            perror(RED "Client socket creation error" RESET);
            continue;
        }

        printf(MAG "New client connected on socket %d\n" RESET, clientSock);
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
                perror(RED "Poll error" RESET);
                exit(1);
            }

            else if (ready > 0) {
                printf("Ready events: %d\n", ready);
                for (pollfd &pfd : ppoll) {
                    if (pfd.revents & POLLIN) {
                        printf("POLLIN on socket %d, revents=%d\n", pfd.fd, pfd.revents);
                        int len = read(pfd.fd, buffer, 16);
                        if (len <= 0) {
                            printf(RED "Poll read error, closing socket %d\n" RESET, pfd.fd);
                            auto pos = find(waitingClients.begin(), waitingClients.end(), pfd.fd);
                            waitingClients.erase(pos, pos+1);
                            close(pfd.fd);
                            pfd = {};
                        } else {
                            printf("Poll on socket %d: %.*s", pfd.fd, len, buffer);
                        }
                    }

                }
            }

            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf(YEL "Time elapsed: %d\n" RESET, ++seconds);

        }
        printf("End countdown\n");
    }
}