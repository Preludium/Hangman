#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <string.h>
using namespace std;

class Client {
    int socket;
    bool status; // false - waiting, true - playing
    string nick;
    int points;
    // bool letters[]; cpp nie zwraca list xD
    vector<bool> letters;

public:
    Client();
    Client(int socket);//, string nick);

    void setSocket(int socket);
    int getSocket();
    void setStatus(bool status);
    bool getStatus();
    void setNick(string nick);
    string getNick();
    void setPoints(int num);
    int getPoints();
    void setLetters(vector<bool> letters);
    vector<bool> getLetters(); 

    void addPoints(int num);
    void sendMsg(string msg);
    void moveToPlaying();
    void moveToWaiting();

    bool operator== (Client &rhs);

};

#endif // CLIENT_H