#include "client.h"

Client::Client() {

}

Client::Client(int socket, string nick) {
    this->socket = socket;
    this->nick = nick;    
    this->status = false;
    this->points = 0;
    this->remaining = MAX_FAILS;
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

void Client::setPoints(int points) {
    this->points = points;
}

int Client::getPoints() {
    return this->points;
}

void Client::setRemaining(int remaining) {
    this->remaining = remaining;
}

int Client::getRemaining() {
    return this->remaining;
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
    // handle socket ignore
    if (this->socket == -1)
        return;

    msg += "\n";
    char m[msg.size()+1];
    strcpy(m, msg.c_str());

    if(send(this->socket, m, sizeof(m), MSG_DONTWAIT) == -1) {
        this->socket = -1;
        printf("Error while writing to client\n");
    }
}

void Client::moveToPlaying() {
    this->status = true;
}

void Client::moveToWaiting() {
    this->status = false;
}

void Client::swap() {
    this->status = !this->status;
}

int Client::notifyFail() {
    string s = BAD;
    s += " " + to_string(this->remaining);
    sendMsg(s);
    printf("%s\n", s.c_str());
    return this->remaining--;
}

void Client::notifyGood(char letter, vector<int> positions) {
    string output = GOOD;
    output += " ";
    output += letter;
    for (auto pos : positions) {
        output += " ";
        output += to_string(pos);
    }
    this->sendMsg(output);
    printf("%s\n", output.c_str());
}

void Client::setLettersViaPositions(char letter, vector<int> positions) {
    for (auto pos: positions)
        this->letters.at(pos) = true;
}

void Client::resetRemaining() {
    this->remaining = MAX_FAILS;
}

bool Client::hasGuessedAll() {
    unsigned int c = 0;
    for (bool l: this->letters) c += l;
    return c == this->letters.size();
}

bool Client::operator== (Client &rhs) {
    if (this->socket == rhs.getSocket() && this->status == rhs.getStatus()) 
        return true;
    return false;
}
