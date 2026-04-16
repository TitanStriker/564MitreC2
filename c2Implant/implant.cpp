// See 

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encoding(const std::string& input) {
    std::string out;
    int val = 0, valb = -6;
    for(unsigned char c : input) {
        val = (val << 8 + c);
        valb += 8;
        while(valb >= 0) {
            out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if(valb > -6) out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while(out.size() % 4) out.push_back('=');
    return out;
}

std::string base64_decode(const std::string& input) {
    std::vector<int> T(256, -1);
    for(int i=0; i < 64; i++) T[B64_CHARS[i]] = i;

    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if(T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

void handleMessage(const std::string& msg, int sock) {
    std::istringstream iss(msg);
    std::string keyword;
    iss >> keyword;

    if(keyword == "HELO") {
        std::string s = "HELLO";
        send(sock, s.c_str(), s.size(), 0);
    } else if(keyword == "EXIT") {
        // throw
    } else if(keyword == "READ") {

    } else if(keyword == "RITE") {

    } else if(keyword == "CMD"){

    } else {
        // ERR
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
            handleMessage(base64_decode(message), clientSocket);
        } catch (...) {
            break;
        }
    }

    close(clientSocket);

    return EXIT_SUCCESS;
}