#include "client.h"

Client::Client() {

}

Client::Client(int socket) {//, string nick) {
    this->socket = socket;
    // this.nick = nick;    
    this->status = false;
    this->points = 0;
    // this.letters;
}

void Client::setSocket(int socket) {
    this->socket = socket;
}

int Client::getSocket() {
    return this->socket;
}

void Client::setStatus(bool status) {
    this->status = status;
}

bool Client::getStatus() {
    return this->status;
}

void Client::setNick(string nick) {
    this->nick = nick;
}

string Client::getNick() {
    return this->nick;
}

void Client::setPoints(int num) {
    this->points = points;
}

int Client::getPoints() {
    return this->points;
}

void Client::setLetters(vector<bool> letters) {
    this->letters = letters;
}

vector<bool> Client::getLetters() {
    return this->letters;
}

void Client::addPoints(int num) {
    this->points += num;
}

void Client::sendMsg(string msg) {
    int n = write(this->socket, msg.c_str(), sizeof(msg));
    if (n == -1) {
        fprintf(stderr, "Writing to %s error : %s", this->nick, strerror(errno));
        // exit(EXIT_FAILURE);
    }
}

void Client::moveToPlaying() {
    this->status = true;
}

void Client::moveToWaiting() {
    this->status = false;
}

// zmienic na porownanie nicku i socketa
bool Client::operator== (Client &rhs) {
    if (this->socket == rhs.getSocket() && this->status == rhs.getStatus()) 
        return true;
    return false;
}
