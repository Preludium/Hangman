#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "client.h"
#include "colors.h"
#include "commands.h"

#define WAITING false
#define PLAYING true
#define MAX_LEN 32
#define MAX_EVENTS 8
#define NICK_TIMEOUT 1000 //ms

using namespace std;
const int one = 1;

int COUNTDOWN_TIME = 5, GAME_TIME = 10;
string ipAddress = "127.0.0.1";
int port = 8080;

int serverSocket;
bool isCountdown = false;
vector<string> database;
vector<Client> clients;

thread newClientsThread, gameThread;
mutex clientsMtx;
condition_variable newClientReady;

bool readConfig(string);
void handleNewConnections();
void handleGame();

// OK
// main
int main(int argc, char** argv) {
    // rand setup
    srand(time(NULL));
    rand(); rand();

    // parse arguments
    if (argc == 3) {
        ipAddress = argv[1];
        try {
            port = stoi(argv[2]);
        } catch(std::exception const & e) {
            printf(RED "Bad args - you should call: ./server [ip address] [port]\n" RESET);
            exit(EXIT_FAILURE);
        }
    }

    if (readConfig("config.txt") == false) {
        printf(RED "Reading config.txt error\n" RESET);
        exit(EXIT_FAILURE);
    }

    printf(MAG "Server configuration: COUNTDOWN_TIME=%d, GAME_TIME=%d\n" RESET, COUNTDOWN_TIME, GAME_TIME);

    fcntl(0, F_SETFL, O_NONBLOCK);
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == -1) {
        perror(RED "Socket creation error" RESET);
        exit(EXIT_FAILURE);
    }

    printf(MAG "Starting server on socket %d\n" RESET, serverSocket);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    addr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*) &addr, sizeof(addr)) == -1) {
        perror(RED "Bind socket error" RESET);
        exit(EXIT_FAILURE);
    }

    printf(MAG "Listening on %s, port %d...\n" RESET, ipAddress.c_str(), port);
    if (listen(serverSocket, 1) == -1) {
        perror(RED "Listen server socket error" RESET);
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);
    newClientsThread = thread(handleNewConnections);
    gameThread = thread(handleGame);

    newClientsThread.join();
    gameThread.join();

    return EXIT_SUCCESS;
}

/// OK
// reads time and words database from a file of a given name
bool readConfig(string filename) {
    ifstream file(filename);
    if (file.good()) {
        string word;

        try {
            file >> word;
            COUNTDOWN_TIME = stoi(word);
            file >> word;
            GAME_TIME = stoi(word);
        }
        catch(std::exception const & e) {
            printf(RED "Config file error - you should pass countdown time [s] and game time [s] in the first line\n" RESET);
            return false;
        }

        while (!file.eof()) {
            file >> word;
            database.push_back(word);
        }
        return true;
    }
    return false;
}

// OK
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

        pollfd fd;
        fd.fd = clientSock;
        fd.events = POLLIN;

        if (poll(&fd, 1, NICK_TIMEOUT) != 1) {
            printf(YEL "Client nick submission timed out, closing socket %d\n" RESET, clientSock);
            close(clientSock);
            continue;
        }

        if((len = read(clientSock, buf, sizeof(buf))) < 0) {
            perror(RED "Client socket read error, closing socket" RESET);
            close(clientSock);
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

                    if (isCountdown)
                        newClient.setStatus(PLAYING);
                    clientsMtx.lock();
                    clients.push_back(newClient);
                    clientsMtx.unlock();
                }
                else {
                    printf(YEL "Nick taken, client connection refused\n" RESET);
                    write(clientSock, REFUSE, sizeof(REFUSE));
                    close(clientSock);
                    continue;
                }
            } else { // bad request
                printf(YEL "Bad nick request from client, closing socket %d\n" RESET, clientSock);
                close(clientSock);
                continue;
            }
        }

        newClientReady.notify_one();
    }
}

/// OK
// wait for at least 2 clients
void waitForClients() {
    printf("Waiting for at least 2 connected clients...\n");

    clientsMtx.lock();
    for (auto client: clients)
        client.sendMsg(WAIT);
    clientsMtx.unlock();

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
        if (client.getStatus() == WAITING)
            ++c;
    return c;
}

/// OK
// counts clients with status of playing
int countPlayingClients() {
    int c=0;
    for (auto client: clients)
        if (client.getStatus() == PLAYING)
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
    string s = GAME;
    s += " " + to_string(len);

    clientsMtx.lock();
    for (auto client : clients) {
        if (client.getStatus() == PLAYING)
            client.sendMsg(s);
    }
    clientsMtx.unlock();
}

/// OK
// inform clients about game over
void notifyGameOver() {
    string s = OVER;

    clientsMtx.lock();
    s += " " + to_string(clients.size());
    vector<string> board = getLeaderboard();

    for (auto client : clients) {
        client.sendMsg(s);
        for (string msg : board)
            client.sendMsg(msg);
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

// OK
// if poll read error occurs, fd will be removed from memory
void handlePollReadError(pollfd &ppoll) {
    printf(YEL "Poll read error, closing socket %d\n" RESET, ppoll.fd);
    clientsMtx.lock();

    int index = ppoll.fd;
    auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});
    clients.erase(pos);

    clientsMtx.unlock();
    close(ppoll.fd);
    ppoll.fd = -1;
}

// OK
// whole countdown handling 
void handleCountdownProcedure() {
    isCountdown = true;

    string s = COUNT;
    s += " " + to_string(COUNTDOWN_TIME);
    
    clientsMtx.lock();
    int iter = 0, clients_size = countWaitingClients();
    pollfd *ppoll = new pollfd[clients_size]{};

    for (auto client: clients) { // poll only waiting clients
        if (client.getStatus() == WAITING) {
            ppoll[iter].fd = client.getSocket();
            ppoll[iter].events = POLLIN;

            printf("%s\n", client.getNick().c_str());
            client.sendMsg(s); // notify waiting client about countdown
            ++iter;
        }
    }
    clientsMtx.unlock();
    char message[MAX_LEN];

    auto end = chrono::system_clock::now() + chrono::seconds(COUNTDOWN_TIME);
    long toWait = chrono::duration_cast<chrono::milliseconds>(end - chrono::system_clock::now()).count();

    printf("Starting countdown of %ds...\n", COUNTDOWN_TIME); 
    while (toWait > 0) {
        int ready = poll(ppoll, clients_size, toWait);
        if (ready == -1) {
            perror(RED "Poll error" RESET);
            exit(1);
        }

        else if (ready > 0) {
            //printf("Ready events: %d\n", ready);
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
                            pos->sendMsg(READY); // notify user
                            ppoll[i].fd = -1;
                        }
                    }
                } else if (ppoll[i].revents & POLLERR || ppoll[i].revents & POLLHUP) {
                    handlePollReadError(ppoll[i]);
                }
            }
        }

        toWait = chrono::duration_cast<chrono::milliseconds>(end - chrono::system_clock::now()).count();
    }

    delete[] ppoll;
    isCountdown = false;
}

// OK
// whole one game handling
void handleGameProcedure() {
    printf("Starting new game...\n");
    int wordNumber = rand() % database.size();
    printf("Chosen word: %s\n", database.at(wordNumber).c_str());

    clientsMtx.lock();
    int iter=0, clients_size = countPlayingClients();
    pollfd *ppoll = new pollfd[clients_size]{};
    for (auto &client: clients) { // poll only playing clients
        if (client.getStatus() == PLAYING) {
            ppoll[iter].fd = client.getSocket();
            ppoll[iter].events = POLLIN;
            client.setLetters(vector<bool>(database.at(wordNumber).length()));
            client.resetRemaining();
            ++iter;
        }
    }
    clientsMtx.unlock();

    printf(CYN "GO!\n" RESET);
    notifyNewGame(database.at(wordNumber).length());

    bool wordGuessed = false;
    char message[MAX_LEN];
    int active_clients = clients_size;

    auto end = chrono::system_clock::now() + chrono::seconds(GAME_TIME);
    long toWait = chrono::duration_cast<chrono::milliseconds>(end - chrono::system_clock::now()).count();

    while (toWait > 0 && !wordGuessed) {
        int ready = poll(ppoll, clients_size, toWait);
        if (ready == -1) {
            perror(RED "Poll error" RESET);
            exit(1);
        }
        else if (ready > 0) {
            //printf("Ready events: %d\n", ready);
            for (int i=0; i<clients_size; ++i) {
                if (ppoll[i].revents & POLLIN) {
                    int len = read(ppoll[i].fd, message, MAX_LEN);
                    if (len <= 0) {
                        handlePollReadError(ppoll[i]);
                        --active_clients;
                    } else {
                        printf(CYN "Poll on socket %d: %.*s\n" RESET, ppoll[i].fd, len, message);
                        clientsMtx.lock();

                        int index = ppoll[i].fd;
                        auto pos = find_if(clients.begin(), clients.end(), [&index](Client& x){return x.getSocket() == index;});

                        clientsMtx.unlock();

                        size_t position = database.at(wordNumber).find(message[0]);
                        if (position != string::npos && !pos->getLetters().at(position)) {
                            vector<int> positions = findPositions(wordNumber, message[0]);
                            pos->addPoints(positions.size());
                            pos->notifyGood(message[0], positions);
                            pos->setLettersViaPositions(message[0], positions);
                            if (pos->hasGuessedAll())
                                wordGuessed = true;

                        } else {
                            int remaining = pos->notifyFail();
                            if (remaining == 0) {
                                --active_clients;
                                ppoll[i].fd = -1;
                            }
                        }
                    }
                } else if (ppoll[i].revents & POLLERR || ppoll[i].revents & POLLHUP) {
                    handlePollReadError(ppoll[i]);
                    --active_clients;
                }
            }
        }

        if (active_clients < 2) {
            printf(CYN "Not enough players left. Interrupting game...\n" RESET);
            break;
        }

        toWait = chrono::duration_cast<chrono::milliseconds>(end - chrono::system_clock::now()).count();
    }

    delete[] ppoll;

    printf(CYN "Game finished\n" RESET);
    notifyGameOver();
    sleep(1);
}

/// OK
// kick inactive clients and return, whether enough clients are left
bool kickInactiveClients() {
    printf("Countdown finished, checking waiting clients...\n");
    clientsMtx.lock();

    for (auto it = clients.begin(); it != clients.end(); ) {
        if (it->getStatus() == WAITING) {
            printf(YEL "Kicking client on socket %d\n" RESET, it->getSocket());
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
        printf("Not enough players, aborting game\n");
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