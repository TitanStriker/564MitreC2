/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Privilege Escalation Pre-Check Implementation (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * Stealth improvements developed with assistance from Claude (Anthropic)
 * For educational use in controlled lab environment only.
 */

#include "privesc_check.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

// Define dirent64 structure manually to avoid header issues
struct linux_dirent64 {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

// Directory type constant
#ifndef DT_DIR
#define DT_DIR 4
#endif

// ============================================================================
// MINIMAL STRING HELPERS (no libc dependencies)
// ============================================================================

static int str_copy(char* dst, const char* src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

static bool str_equals(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return false;
        i++;
    }
    return a[i] == b[i];
}

static bool str_contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    
    int hay_len = 0;
    while (haystack[hay_len]) hay_len++;
    
    int needle_len = 0;
    while (needle[needle_len]) needle_len++;
    
    if (needle_len > hay_len) return false;
    
    for (int i = 0; i <= hay_len - needle_len; i++) {
        bool match = true;
        for (int j = 0; j < needle_len; j++) {
            if (haystack[i + j] != needle[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    
    return false;
}

// ============================================================================
// STEALTH-ENHANCED DIRECTORY CHECKS
// ============================================================================

static bool check_directory_writable_stealthy(const char* path) {
    struct stat st;
    
    if (syscall(SYS_stat, path, &st) != 0) {
        return false;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    
    unsigned int uid = syscall(SYS_getuid);
    unsigned int gid = syscall(SYS_getgid);
    
    bool writable = false;
    
    if (st.st_uid == uid && (st.st_mode & S_IWUSR)) {
        writable = true;
    }
    else if (st.st_gid == gid && (st.st_mode & S_IWGRP)) {
        writable = true;
    }
    else if (st.st_mode & S_IWOTH) {
        writable = true;
    }
    
    return writable;
}

// ============================================================================
// STEALTH-ENHANCED TAR CHECK
// ============================================================================

static bool check_tar_available_stealthy() {
    struct stat st;
    
    const char* tar_paths[] = {
        "/bin/tar",
        "/usr/bin/tar",
        nullptr
    };
    
    for (int i = 0; tar_paths[i] != nullptr; i++) {
        if (syscall(SYS_stat, tar_paths[i], &st) == 0) {
            if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                return true;
            }
        }
    }
    
    return false;
}

// ============================================================================
// STEALTH-ENHANCED CRON CHECK
// ============================================================================

static bool check_cron_running_stealthy() {
    // Strategy 1: Check /proc/self/cgroup
    int cgroup_fd = syscall(SYS_open, "/proc/self/cgroup", O_RDONLY);
    if (cgroup_fd >= 0) {
        char cgroup_buf[512];
        long bytes = syscall(SYS_read, cgroup_fd, cgroup_buf, sizeof(cgroup_buf) - 1);
        syscall(SYS_close, cgroup_fd);
        
        if (bytes > 0) {
            cgroup_buf[bytes] = '\0';
            if (str_contains(cgroup_buf, "cron.service")) {
                return true;
            }
        }
    }
    
    // Strategy 2: Check PID files
    struct stat st;
    const char* cron_pid_files[] = {
        "/run/crond.pid",
        "/var/run/crond.pid",
        "/run/cron.pid",
        "/var/run/cron.pid",
        nullptr
    };
    
    for (int i = 0; cron_pid_files[i]; i++) {
        if (syscall(SYS_stat, cron_pid_files[i], &st) == 0) {
            return true;
        }
    }
    
    // Strategy 3: Check only LOW PIDs
    const int max_pid_check = 300;
    
    for (int pid = 1; pid <= max_pid_check; pid++) {
        char comm_path[32];
        comm_path[0] = '/';
        comm_path[1] = 'p';
        comm_path[2] = 'r';
        comm_path[3] = 'o';
        comm_path[4] = 'c';
        comm_path[5] = '/';
        
        int path_pos = 6;
        char pid_str[16];
        int pid_len = 0;
        
        if (pid < 10) {
            pid_str[pid_len++] = '0' + pid;
        } else {
            int temp_pid = pid;
            char temp[16];
            int temp_pos = 0;
            while (temp_pid > 0) {
                temp[temp_pos++] = '0' + (temp_pid % 10);
                temp_pid /= 10;
            }
            for (int j = temp_pos - 1; j >= 0; j--) {
                pid_str[pid_len++] = temp[j];
            }
        }
        pid_str[pid_len] = '\0';
        
        for (int j = 0; j < pid_len; j++) {
            comm_path[path_pos++] = pid_str[j];
        }
        comm_path[path_pos++] = '/';
        comm_path[path_pos++] = 'c';
        comm_path[path_pos++] = 'o';
        comm_path[path_pos++] = 'm';
        comm_path[path_pos++] = 'm';
        comm_path[path_pos] = '\0';
        
        int comm_fd = syscall(SYS_open, comm_path, O_RDONLY);
        if (comm_fd >= 0) {
            char comm_buf[32];
            long bytes = syscall(SYS_read, comm_fd, comm_buf, sizeof(comm_buf) - 1);
            syscall(SYS_close, comm_fd);
            
            if (bytes > 0) {
                comm_buf[bytes] = '\0';
                
                if (str_equals(comm_buf, "cron\n") || 
                    str_equals(comm_buf, "crond\n")) {
                    return true;
                }
            }
        }
        
        if (pid % 50 == 0) {
            struct timespec micro_sleep = {0, 1000000};
            syscall(SYS_nanosleep, &micro_sleep, nullptr);
        }
    }
    
    // Strategy 4: Check systemd timer directory
    if (syscall(SYS_access, "/etc/systemd/system/timers.target.wants", F_OK) == 0) {
        return true;
    }
    
    return false;
}

// ============================================================================
// STEALTH-ENHANCED USERNAME RETRIEVAL
// ============================================================================

static void get_username_stealthy(char* buf, int max_len) {
    unsigned int uid = syscall(SYS_getuid);
    
    if (uid == 0) {
        str_copy(buf, "0", max_len);
        return;
    }
    
    char uid_str[16];
    int pos = 0;
    unsigned int temp_uid = uid;
    
    char temp[16];
    int temp_pos = 0;
    while (temp_uid > 0) {
        temp[temp_pos++] = '0' + (temp_uid % 10);
        temp_uid /= 10;
    }
    
    for (int i = temp_pos - 1; i >= 0; i--) {
        uid_str[pos++] = temp[i];
    }
    uid_str[pos] = '\0';
    
    str_copy(buf, uid_str, max_len);
}

// ============================================================================
// MAIN CHECK FUNCTION (STEALTH-ENHANCED)
// ============================================================================

PrivEscConditions check_privesc_viability() {
    PrivEscConditions conditions;
    memset(&conditions, 0, sizeof(conditions));
    
    conditions.current_uid = syscall(SYS_getuid);
    
    get_username_stealthy(conditions.username, sizeof(conditions.username));
    
    conditions.backups_dir_exists = (syscall(SYS_access, "/tmp/backups", F_OK) == 0);
    conditions.backups_dir_writable = check_directory_writable_stealthy("/tmp/backups");
    
    conditions.tar_available = check_tar_available_stealthy();
    
    conditions.cron_running = check_cron_running_stealthy();
    
    return conditions;
}
