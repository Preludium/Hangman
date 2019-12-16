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
#include <sys/epoll.h>
#include <algorithm>
#include <string.h>
#include <fstream>

#define ACCEPT "ACCEPT\n"
#define REFUSE "REFUSE\n"
#define KICK "KICK\n"

#define COUNTDOWN_TIME 5
#define MAX_LEN 8
#define MAX_EVENTS 8

using namespace std;
const int one = 1;

int serverSocket;
bool isCountdown = false;
vector<int> waitingClients, playingClients;
vector<string> database;

thread newClientsThread, gameThread;
mutex clientsMtx, countdownMtx;

bool readDatabase(string filename) {
    ifstream file(filename);
    if (file.good()) {
        string word;
        while (!file.eof()) {
            file >> word;
            database.push_back(word);
        }
        return true;
    }
    return false;
}

void handleNewConnections() {
    int clientSock;
    while(true) {
        clientSock = accept(serverSocket, nullptr, nullptr);

        if (clientSock == -1) {
            perror("Client socket creation error");
            continue;
        }

        printf("New client connected on socket %d\n", clientSock);
        write(clientSock, ACCEPT, sizeof(ACCEPT));

        countdownMtx.lock();
        if (isCountdown) {
            clientsMtx.lock();
            playingClients.push_back(clientSock);
            clientsMtx.unlock();
        } else {
            clientsMtx.lock();
            waitingClients.push_back(clientSock);
            clientsMtx.unlock();
        }
        countdownMtx.unlock();
        
    }
}

void handleGame() {
    while (true) {
        // wait for at least 2 clients
        printf("Waiting for at least 2 connected clients\n");
        int connectedClients = 0;
        while (connectedClients < 2) {
            clientsMtx.lock();
            connectedClients = waitingClients.size() + playingClients.size();
            clientsMtx.unlock();
        }

        printf("%d connected clients, preparing for a new game\n", connectedClients);

        // notify countdown start
        countdownMtx.lock();
        isCountdown = true;
        countdownMtx.unlock();

        // waitingClients moved from waiting room to new game
        // playingClients moved from last game to waiting room
        clientsMtx.lock();
        swap(waitingClients, playingClients);
        clientsMtx.unlock();

        // set epoll for a new session countdown
        int newGameEpoll = epoll_create1(0);
        epoll_event eevent, events[MAX_EVENTS];
        eevent.events = EPOLLIN;
        eevent.data.fd = serverSocket;

        clientsMtx.lock();
        for (int i=0; i<waitingClients.size(); ++i)
            epoll_ctl(newGameEpoll, EPOLL_CTL_ADD, waitingClients.at(i), &eevent);
        clientsMtx.unlock();

        // set variables for countdown
        int eventCount = 0;
        char message[MAX_LEN];
        clock_t begClk = clock();

        // run countdown to start new game session
        printf("Starting countdown\n");
        int seconds = 0, elapsed = (clock() - begClk) / CLOCKS_PER_SEC;

        while (elapsed < COUNTDOWN_TIME) {
            eventCount = epoll_wait(newGameEpoll, events, MAX_EVENTS, 0);
            
            for (int i=0; i<eventCount; ++i) {
                int len = read(events[i].data.fd, message, MAX_LEN);
                if (len == -1) {
                    // ignore further client polling
                    epoll_ctl(newGameEpoll, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    continue;
                }

                printf("Request from socket %d: %s\n", events[i].data.fd, message);

                // check request content
                if (strncmp(message, ACCEPT, sizeof(ACCEPT)) == 0) {
                    
                    // move client from waiting to playing
                    clientsMtx.lock();
                    auto pos = find(waitingClients.begin(), waitingClients.end(), events[i].data.fd);
                    if (pos != waitingClients.end()) waitingClients.erase(pos);
                    playingClients.push_back(events[i].data.fd);
                    clientsMtx.unlock();
                    
                    // delete client's epoll
                    epoll_ctl(newGameEpoll, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                }
                
            }

            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf("Time elapsed: %d\n", ++seconds);

        }

        close(newGameEpoll);

        // notify countdown stop
        countdownMtx.lock();
        isCountdown = false;
        countdownMtx.unlock();

        // kick waiting clients that didn't declare their readiness
        printf("Checking waiting clients\n");
        clientsMtx.lock();

        if (waitingClients.size() == 0)
            printf("All clients ready\n");
        for (int i=0; i<waitingClients.size(); ++i) {
            printf("Kicking client on socket %d\n", waitingClients.at(i));
            write(waitingClients.at(i), KICK, sizeof(KICK));
        }
        waitingClients.clear();

        // if not enough players
        if (playingClients.size() < 2) {
            printf("Not enough players, canceling game\n");
            clientsMtx.unlock();
            continue;
        }

        clientsMtx.unlock();

        printf("Starting new game\n");
        // TODO: choose random word
        // TODO: inform players, that a new game started
        printf("GO!\n");
        // TODO: wait for events and give appropriate answer to clients
        // TODO: if no players stay active, finish the game
        printf("Game finished\n");
        // TODO: inform players, that the game has finished and send leaderboard
    }
}

int main(int argc, char** argv) {
    // TODO: parse arguments: 1 - database file, 2 - settings file (optional)
    readDatabase("database.txt");

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