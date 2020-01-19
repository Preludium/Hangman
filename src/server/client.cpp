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

// TODO: needs checking of send error handling 
//  (for now: server dies after sending message to disconnected client)
void Client::sendMsg(string msg) {
    msg += "\n";
    char m[msg.size()+1];
    strcpy(m, msg.c_str());
    if(send(this->socket, m, sizeof(m), MSG_DONTWAIT) == -1) {
        this->socket = -1;
        perror("Writing to client error");
    }

    // if (n == -1) {
    //     fprintf(stderr, "Writing to %s error : %s", this->nick.c_str(), strerror(errno));
        // exit(EXIT_FAILURE);
    // }
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

int Client::noteFail() {
    return --this->remaining;
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

// zmienic na porownanie nicku i socketa
bool Client::operator== (Client &rhs) {
    if (this->socket == rhs.getSocket() && this->status == rhs.getStatus()) 
        return true;
    return false;
}
