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
#include <fcntl.h>

#define ACCEPT "ACCEPT\n"
#define REFUSE "REFUSE\n"
#define KICK "KICK\n"
#define COUNT "COUNT\n"
#define GAME "GAME\n"
#define OVER "OVER\n"

#define COUNTDOWN_TIME 5
#define GAME_TIME 10
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

bool readDatabase(string);
void handleNewConnections();
void handleGame();

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

// wait for at least 2 clients
void waitForClients() {
    printf("Waiting for at least 2 connected clients\n");
    int connectedClients = 0;
    while (connectedClients < 2) {
        clientsMtx.lock();
        connectedClients = waitingClients.size() + playingClients.size();
        clientsMtx.unlock();
    }
    printf("%d connected clients, preparing for a new game\n", connectedClients);
}

// notify whether countdown is on or not
void notifyCountdown(bool value) {
    countdownMtx.lock();
    isCountdown = value;
    countdownMtx.unlock();
}

// swap clients between waiting room and game room
void swapClients() {
    clientsMtx.lock();
    swap(waitingClients, playingClients);
    clientsMtx.unlock();
}

// set epoll for a new session countdown
int setUpCountdownEpoll(vector<int> *clients) {
    int newEpoll = epoll_create1(0);
    epoll_event eevent;
    eevent.events = EPOLLIN;
    eevent.data.fd = serverSocket;

    clientsMtx.lock();
    for (int i=0; i<clients->size(); ++i)
        epoll_ctl(newEpoll, EPOLL_CTL_ADD, clients->at(i), &eevent);
    clientsMtx.unlock();

    return newEpoll;
}

// delete client from waiting room
void deleteClient(epoll_event event) {
    clientsMtx.lock();
    auto pos = find(waitingClients.begin(), waitingClients.end(), event.data.fd);
    if (pos != waitingClients.end())
        waitingClients.erase(pos);
    clientsMtx.unlock();
    close(event.data.fd);
}

// move client from waiting to playing
void moveClient(epoll_event event) {
    clientsMtx.lock();
    auto pos = find(waitingClients.begin(), waitingClients.end(), event.data.fd);
    if (pos != waitingClients.end()) waitingClients.erase(pos);
    playingClients.push_back(event.data.fd);
    clientsMtx.unlock();
}

// kick inactive clients and return, whether enough clients are left
bool kickInactiveClients() {
    printf("Checking waiting clients\n");
    clientsMtx.lock();

    if (waitingClients.size() == 0)
        printf("All clients ready\n");
    for (int i=0; i<waitingClients.size(); ++i) {
        printf("Kicking client on socket %d\n", waitingClients.at(i));
        write(waitingClients.at(i), KICK, sizeof(KICK));
        close(waitingClients.at(i));
    }
    waitingClients.clear();

    // if not enough players
    if (playingClients.size() < 2) {
        printf("Not enough players, canceling game\n");
        clientsMtx.unlock();
        return false;
    }

    clientsMtx.unlock();
    return true;
}

// inform clients about new game
void notifyNewGame() {
    // here mutex not necessary
    for (int i=0; i<playingClients.size(); ++i)
        write(playingClients.at(i), GAME, sizeof(GAME));
}

// inform clients about game over
void notifyGameOver() {
    // here mutex not necessary
    for (int i=0; i<playingClients.size(); ++i)
        write(playingClients.at(i), OVER, sizeof(OVER));
}

void handleGame() {
    while (true) {
        waitForClients();

        notifyCountdown(true);
        swapClients();
    
        int countdownEpoll = setUpCountdownEpoll(&waitingClients);
        epoll_event events[MAX_EVENTS];
        int eventCount = 0, seconds = 0, elapsed;
        char message[MAX_LEN];

        printf("Starting countdown\n");
        clock_t begClk = clock();
        elapsed = (clock() - begClk) / CLOCKS_PER_SEC;

        while (elapsed < COUNTDOWN_TIME) {
            eventCount = epoll_wait(countdownEpoll, events, MAX_EVENTS, 0);

            for (int i=0; i<eventCount; ++i) {
                int len = read(events[i].data.fd, message, MAX_LEN);
                if (len == -1) {
                    close(events[i].data.fd);
                    deleteClient(events[i]);
                    epoll_ctl(countdownEpoll, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    continue;
                }

                printf("Request from socket %d: %s\n", events[i].data.fd, message);
                if (strncmp(message, ACCEPT, sizeof(ACCEPT)) == 0) {
                    moveClient(events[i]);
                    epoll_ctl(countdownEpoll, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                }
                
            }

            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf("Time elapsed: %d\n", ++seconds);

        }

        close(countdownEpoll);
        notifyCountdown(false);

        if (!kickInactiveClients())
            continue;

        printf("Starting new game\n");
        int wordNumber = rand() % database.size();
        
        int gameEpoll = setUpCountdownEpoll(&waitingClients);
        eventCount = 0; seconds = 0;

        printf("GO!\n");
        notifyNewGame();

        begClk = clock();
        elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
        while (elapsed < GAME_TIME) {
            // TODO: wait for events and give appropriate answer to clients
            // TODO: if no players stay active, finish the game
            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
        }

        close(gameEpoll);
        printf("Game finished\n");
        notifyGameOver();
    }
}
