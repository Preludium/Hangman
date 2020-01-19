#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "commands.h"

#define MAX_FAILS 8
using namespace std;

class Client {
    int socket;
    bool status; // false - waiting, true - playing
    string nick;
    int points;
    int remaining;
    vector<bool> letters;

public:
    Client();
    Client(int socket, string nick);

    void setSocket(int socket);
    int getSocket();
    void setStatus(bool status);
    bool getStatus();
    void setNick(string nick);
    string getNick();
    void setPoints(int num);
    int getPoints();
    void setRemaining(int remaining);
    int getRemaining();
    void setLetters(vector<bool> letters);
    vector<bool> getLetters(); 

    void addPoints(int num);
    void sendMsg(string msg);
    void moveToPlaying();
    void moveToWaiting();
    void swap();
    int notifyFail();
    void notifyGood(char letter, vector<int> positions);
    void setLettersViaPositions(char letter, vector<int> positions);
    void resetRemaining();
    bool hasGuessedAll();

    bool operator== (Client &rhs);

};

#endif // CLIENT_H