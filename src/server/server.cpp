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
#include <condition_variable>

#include "client.h"
#include "colors.h"
#include "commands.h"

#define COUNTDOWN_TIME 5
#define GAME_TIME 10
#define MAX_LEN 16
#define MAX_EVENTS 8

using namespace std;
const int one = 1;

int serverSocket;
vector<string> database;
vector<Client> clients;

thread newClientsThread, gameThread;
mutex clientsMtx;
condition_variable newClientReady;

bool readDatabase(string);
void handleNewConnections();
void handleGame();

int main(int argc, char** argv) {
    // rand setup
    srand(time(NULL));
    rand(); rand();

    // TODO: parse arguments: 1 - database file, 2 - settings file (optional)
    if (readDatabase("database.txt") == false) {
        printf(RED "Reading database.txt error\n" RESET);
        exit(EXIT_FAILURE);
    }

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
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*) &addr, sizeof(addr)) == -1) {
        perror(RED "Bind socket error" RESET);
        exit(EXIT_FAILURE);
    }

    printf("Listening on 127.0.0.1, port 8080\n");
    if (listen(serverSocket, 1) == -1) {
        perror(RED "Listen server socket error" RESET);
        exit(EXIT_FAILURE);
    }

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

// check if given nick is available
bool checkNick(string nick) {

    for(auto client : clients) {
        if (client.getNick().compare(nick) == 0)
            return false;
    }

    return true;
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

        char buf[MAX_LEN];
        int len;

        if((len = read(clientSock, buf, sizeof(buf))) <= 0) {
            perror(RED "Client socket read error" RESET);
            continue;
        }
        else {
            string nick(buf, len);
            if (nick.find("NICK") != string::npos) {
                nick = nick.substr(5);
                if (checkNick(nick)) {
                    Client newClient(clientSock, nick);

                    printf(GRN "New client connected on socket %d\n" RESET, clientSock);
                    newClient.sendMsg(ACCEPT);

                    clientsMtx.lock();
                    clients.push_back(newClient);
                    clientsMtx.unlock();
                }
                else {
                    printf(RED "Nick taken, client connection refused\n" RESET);
                    write(clientSock, REFUSE, sizeof(REFUSE));
                    close(clientSock);
                }
            }
        }

        newClientReady.notify_one();

    }
}

/// OK
// wait for at least 2 clients
void waitForClients() {
    printf("Waiting for at least 2 connected clients...\n");
    
    int connectedClients = 0;
    unique_lock<mutex> lck(clientsMtx);
    newClientReady.wait(lck, [&]{return ( connectedClients = clients.size()) >= 2; });

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

// OK
// get sorted leaderboard by points, with PLAYER prefixes
vector<string> getLeaderboard() {
    vector<string> output;
    for (auto client : clients) 
        output.push_back("PLAYER " + client.getNick() + " " + to_string(client.getPoints()));
    return output;
}

/// OK
// inform clients about new game
void notifyNewGame(int len) {
    char s[MAX_LEN];
    sprintf(s, "%s %d", GAME, len);

    clientsMtx.lock();
    for (auto client : clients) {
        if (client.getStatus())
            client.sendMsg(s);
    }
    clientsMtx.unlock();
}

/// OK
// inform clients about game over
void notifyGameOver() {
    char s[MAX_LEN];

    clientsMtx.lock();
    sprintf(s, "%s %d", OVER, countPlayingClients());
    vector<string> board = getLeaderboard();

    for (auto client : clients) {
        if (client.getStatus()) {
            client.sendMsg(s);
            for (string msg : board)
                client.sendMsg(msg);
        }
    }
    clientsMtx.unlock();
}

// OK
// find all indices of a given letter in word from db
vector<int> findPositions(int wordNumber, char letter) {
    int num = 0;
    vector<int> output;
    for (char &c : database.at(wordNumber)) {
        if (c == letter)
            output.push_back(num);
        ++num;
    }
    return output;
}

// if poll read error occurs, fd will be removed from memory
void handlePollReadError(pollfd &ppoll) {
    printf(RED "Poll read error, closing socket %d\n" RESET, ppoll.fd);
    clientsMtx.lock();

    int index = ppoll.fd;
    auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
    clients.erase(pos);

    clientsMtx.unlock();
    close(ppoll.fd);
    ppoll = {};
}

// OK
// whole countdown handling 
void handleCountdownProcedure() {
    char s[MAX_LEN];
    clientsMtx.lock();
    int iter = 0, clients_size = countWaitingClients();
    pollfd *ppoll = new pollfd[clients_size]{};
    for (auto client: clients) { // poll only waiting clients
        if (!client.getStatus()) { // TODO: compare with const value
            ppoll[iter].fd = client.getSocket();
            ppoll[iter].events = POLLIN;
            ++iter;

            sprintf(s, "%s %d", COUNT, COUNTDOWN_TIME);
            client.sendMsg(s); // notify waiting client about countdown
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
                        handlePollReadError(ppoll[i]);
                    } else {
                        printf("Poll on socket %d: %.*s\n", ppoll[i].fd, len, message);
                        if (strncmp(message, READY, sizeof(READY)-1) == 0) {
                            clientsMtx.lock();

                            int index = ppoll[i].fd;
                            auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
                            if (pos != clients.end()) pos->moveToPlaying();

                            clientsMtx.unlock();
                            pos->sendMsg(ACCEPT); // notify user
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
}

// TODO
// whole one game handling
void handleGameProcedure() {
    printf("Starting new game...\n");
    int wordNumber = rand() % database.size();
    printf("Chosen word: %s\n", database.at(wordNumber).c_str());

    clientsMtx.lock();
    int iter=0, clients_size = countPlayingClients();
    pollfd *ppoll = new pollfd[clients_size]{};
    for (auto client: clients) { // poll only playing clients
        if (client.getStatus()) { // TODO: compare with const value
            ppoll[iter].fd = client.getSocket();
            ppoll[iter].events = POLLIN;
            ++iter;
        }
    }
    clientsMtx.unlock();

    printf("GO!\n");
    notifyNewGame(database.at(wordNumber).length());

    char message[MAX_LEN];
    clock_t begClk = clock();
    int seconds = 0, elapsed = (clock() - begClk) / CLOCKS_PER_SEC;
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
                        handlePollReadError(ppoll[i]);
                    } else {
                        printf("Poll on socket %d: %.*s\n", ppoll[i].fd, len, message);
                        clientsMtx.lock();

                        int index = ppoll[i].fd;
                        auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});

                        clientsMtx.unlock();

                        if (database.at(wordNumber).find(message[0]) != string::npos) {
                            vector<int> positions = findPositions(wordNumber, message[0]);
                            pos->addPoints(positions.size());
                            pos->notifyGood(message[0], positions);

                        } else {
                            int remaining = pos->noteFail();
                            char s[MAX_LEN];
                            sprintf(s, "%s %d", BAD, remaining);
                            pos->sendMsg(s);

                            if (remaining == 0) {
                                clientsMtx.lock();
                                if (pos != clients.end()) pos->moveToWaiting();
                                clientsMtx.unlock();
                                ppoll[i] = {};
                            }
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
// handles whole game thread
void handleGame() {
    while (true) {
        waitForClients(); // wait for at least 2 clients
        swapClients(); // swap waiting with playing
        handleCountdownProcedure(); // poll clients on countdown
        if (!kickInactiveClients()) continue; // kick not ready clients
        handleGameProcedure(); // create game and poll clients
    }
}