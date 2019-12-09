#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <condition_variable>

#define ACCEPT "ACCEPT"
#define REFUSE 

using namespace std;
const int one = 1;

int sock;
vector<int> waitingClients, playingClients;
// cykl: waiting > connected > playing

thread newClientsThread, gameThread;
mutex waitingClientMtx;
condition_variable cv;

void handleNewConnections() {
    int clientSock;
    while(true) {
        clientSock = accept(sock, nullptr, nullptr);

        if (clientSock == -1) {
            perror("Client socket creation error");
            continue;
        }

        write(clientSock, ACCEPT, sizeof(ACCEPT));

        unique_lock<mutex> lock(waitingClientMtx);
        cv.wait(lock);
        waitingClients.push_back(clientSock);
        
    }
}

void handleGame() {
    vector<int> newClients;
    while (true) {
        // Add clients from last session to new session
        newClients = waitingClients;
        waitingClients.clear();
        for (int i = 0; i < playingClients.size(); ++i)
            waitingClients.push_back(playingClients.at(i));
        playingClients = newClients;
        
        // 
    }
}

int main() {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    if (bind(sock, (sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("Bind socket error");
        exit(EXIT_FAILURE);
    }

    listen(sock, 1);

    newClientsThread = thread(handleNewConnections);
    gameThread = thread(handleGame);

    newClientsThread.join();

    return 0;
}