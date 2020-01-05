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
#include <poll.h>
#include "client.h"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define ACCEPT "ACCEPT"
#define REFUSE "REFUSE"
#define KICK "KICK"
#define COUNT "COUNT"
#define GAME "GAME"
#define OVER "OVER"
// #define LETTER "LETTER\n"

#define COUNTDOWN_TIME 5
#define GAME_TIME 10
#define MAX_LEN 8
#define MAX_EVENTS 8

using namespace std;
const int one = 1;

int serverSocket;
vector<string> database;
vector<Client> clients;

thread newClientsThread, gameThread;
mutex clientsMtx;

bool readDatabase(string);
void handleNewConnections();
void handleGame();

int main(int argc, char** argv) {
    // rand setup
    srand(time(NULL));
    rand(); rand();

    // TODO: parse arguments: 1 - database file, 2 - settings file (optional)
    readDatabase("database.txt");

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

/// OK
// reads words database from a file of a given name
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

/// OK
// handles each new connection to server
void handleNewConnections() {
    while(true) {
        int clientSock = accept(serverSocket, nullptr, nullptr);

        if (clientSock == -1) {
            perror(RED "Client socket creation error" RESET);
            continue;
        }

        Client newClient(clientSock);

        printf(GRN "New client connected on socket %d\n" RESET, clientSock);
        newClient.sendMsg(ACCEPT);

        clientsMtx.lock();
        clients.push_back(newClient);
        clientsMtx.unlock();

    }
}

/// OK
// wait for at least 2 clients
void waitForClients() {
    printf("Waiting for at least 2 connected clients...\n");
    int connectedClients = 0;
    while (connectedClients < 2) {
        clientsMtx.lock();
        connectedClients = clients.size();
        clientsMtx.unlock();
    }
    printf("%d connected clients, preparing for a new game...\n", connectedClients);
}

/// OK
// swap clients between waiting room and game room
void swapClients() {
    clientsMtx.lock();
    for(auto &client : clients)
        client.swap();
    clientsMtx.unlock();
}

/// UNUSED
// delete client from waiting room
void deleteClient(pollfd* pfd) {
    clientsMtx.lock();
    // auto pos = find(waitingClients.begin(), waitingClients.end(), pfd->fd);
    // waitingClients.erase(pos, pos+1);
    clientsMtx.unlock();
    close(pfd->fd);
    pfd = {};
}

/// OK
// kick inactive clients and return, whether enough clients are left
bool kickInactiveClients() {
    printf("Countdown finished, checking waiting clients...\n");
    clientsMtx.lock();

    for (auto it = clients.begin(); it != clients.end(); ) {
        if (!it->getStatus()) {
            printf("Kicking client on socket %d\n", it->getSocket());
            it->sendMsg(KICK);
            close(it->getSocket());
            it = clients.erase(it);
        }
        else {
            ++it;
        }
    }

    // if not enough players
    if (clients.size() < 2) {
        printf("Not enough players, canceling game\n");
        clientsMtx.unlock();
        return false;
    }

    clientsMtx.unlock();
    return true;
}

/// OK
// inform clients about new game
void notifyNewGame() {
    clientsMtx.lock();
    for (auto client : clients) {
        if (client.getStatus())
            client.sendMsg(GAME);
    }
    clientsMtx.unlock();
}

/// OK
// inform clients about game over
void notifyGameOver() {
    clientsMtx.lock();
    for (auto client : clients) {
        if (client.getStatus())
            client.sendMsg(OVER);
    }
    clientsMtx.unlock();
}

/// OK
// counts clients with status of waiting
int countWaitingClients() {
    int c=0;
    for (auto client: clients)
        if (!client.getStatus())
            ++c;
    return c;
}

/// OK
// counts clients with status of playing
int countPlayingClients() {
    int c=0;
    for (auto client: clients)
        if (client.getStatus())
            ++c;
    return c;
}

/// NEEDS CHECK
// handles whole game procedure
void handleGame() {// countdown blocks msgs received from clients
    while (true) {
        waitForClients();
        swapClients();
    
        clientsMtx.lock();
        int iter = 0, clients_size = countWaitingClients();
        pollfd *ppoll = new pollfd[clients_size]{};
        for (auto client: clients) { // poll only waiting clients
            if (!client.getStatus()) {
                ppoll[iter].fd = client.getSocket();
                ppoll[iter].events = POLLIN;
                ++iter;
            }
        }
        clientsMtx.unlock();

        char message[MAX_LEN];
        clock_t begClk = clock();
        int seconds = 0, elapsed = (clock() - begClk) / CLOCKS_PER_SEC;

        printf("Starting countdown...\n"); 
        while (elapsed < COUNTDOWN_TIME) {
            int ready = poll(ppoll, clients_size, 0);
            if (ready == -1) {
                perror(RED "Poll error" RESET);
                exit(1);
            }

            else if (ready > 0) {
                printf("Ready events: %d\n", ready);
                for (int i=0; i<clients_size; ++i) {
                    if (ppoll[i].revents & POLLIN) {
                        int len = read(ppoll[i].fd, message, MAX_LEN);
                        if (len <= 0) {
                            printf(RED "Poll read error, closing socket %d\n" RESET, ppoll[i].fd);
                            clientsMtx.lock();

                            int index = ppoll[i].fd;
                            auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
                            clients.erase(pos, pos+1);

                            clientsMtx.unlock();
                            close(ppoll[i].fd);
                            ppoll[i] = {};
                            //
                        } else {
                            printf("Poll on socket %d: %.*s\n", ppoll[i].fd, len, message);
                            if (strncmp(message, ACCEPT, sizeof(ACCEPT)) == 0) {
                                clientsMtx.lock();

                                int index = ppoll[i].fd;
                                auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
                                if (pos != clients.end()) pos->moveToPlaying();

                                clientsMtx.unlock();
                                ppoll[i] = {};
                            }
                        }
                    }
                }
            }

            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf(YEL "Time elapsed: %d\n" RESET, ++seconds);

        }

        delete[] ppoll;
        
        if (!kickInactiveClients())
            continue;

        printf("Starting new game...\n");
        int wordNumber = rand() % database.size();
        printf("Chosen word: %s\n", database.at(wordNumber).c_str());

        // jansmi: checked, good to this point
        //     after: not checked yet

        clientsMtx.lock();
        clients_size = countPlayingClients();
        ppoll = new pollfd[clients_size]{};
        for (auto client: clients) { // poll only playing clients
            if (!client.getStatus()) {
                ppoll[iter].fd = client.getSocket();
                ppoll[iter].events = POLLIN;
                ++iter;
            }
        }
        clientsMtx.unlock();

        printf("GO!\n");
        notifyNewGame();

        begClk = clock();
        seconds = 0; elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
        while (elapsed < GAME_TIME) {
            // TODO: wait for events and give appropriate answer to clients
            // TODO: if no players stay active, finish the game
            int ready = poll(ppoll, clients_size, 0);
            if (ready == -1) {
                perror(RED "Poll error" RESET);
                exit(1);
            }
            else if (ready > 0) {
                printf("Ready events: %d\n", ready);
                for (int i=0; i<clients_size; ++i) {
                    if (ppoll[i].revents & POLLIN) {
                        int len = read(ppoll[i].fd, message, MAX_LEN);
                        if (len <= 0) {
                            printf(RED "Poll read error, closing socket %d\n" RESET, ppoll[i].fd);
                            clientsMtx.lock();

                            int index = ppoll[i].fd;
                            auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
                            clients.erase(pos, pos+1);

                            clientsMtx.unlock();
                            close(ppoll[i].fd);
                            ppoll[i] = {};
                        } else {
                            printf("Poll on socket %d: %.*s", ppoll[i].fd, len, message);
                            if (database.at(wordNumber).find(message[0]) != std::string::npos) {

                            }
                        }
                    }
                }
            }
            elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
            if (elapsed > seconds)
                printf(YEL "Time elapsed: %d\n" RESET, ++seconds);
        }

        delete[] ppoll;

        printf("Game finished\n");
        notifyGameOver();
        
    }
}