/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Privilege Escalation Pre-Check Implementation (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * Stealth improvements developed with assistance from Claude (Anthropic) for:
 * - Read-only directory checks (no file creation)
 * - Targeted process checks (minimal /proc enumeration)
 * - Randomized timing patterns
 * - Minimal syscall footprint
 * 
 * For educational use in controlled lab environment only.
 */

#include "privesc_check.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/dirent.h>
#include <cstring>

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

static bool is_numeric(const char* str) {
    if (!str || !str[0]) return false;
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// ============================================================================
// STEALTH-ENHANCED DIRECTORY CHECKS
// ============================================================================

// Check if directory exists and is writable WITHOUT creating test files
// Uses stat() permission checks instead of actual file creation
static bool check_directory_writable_stealthy(const char* path) {
    struct stat st;
    
    // Check if directory exists
    if (syscall(SYS_stat, path, &st) != 0) {
        return false;
    }
    
    // Verify it's a directory
    if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    
    // Get current UID/GID
    unsigned int uid = syscall(SYS_getuid);
    unsigned int gid = syscall(SYS_getgid);
    
    // Check permissions based on ownership
    bool writable = false;
    
    // Owner write permission
    if (st.st_uid == uid && (st.st_mode & S_IWUSR)) {
        writable = true;
    }
    // Group write permission
    else if (st.st_gid == gid && (st.st_mode & S_IWGRP)) {
        writable = true;
    }
    // Other write permission
    else if (st.st_mode & S_IWOTH) {
        writable = true;
    }
    
    return writable;
}

// ============================================================================
// STEALTH-ENHANCED TAR CHECK
// ============================================================================

// Check if tar binary is available using stat() only (no execution)
static bool check_tar_available_stealthy() {
    struct stat st;
    
    // Check common tar locations
    const char* tar_paths[] = {
        "/bin/tar",
        "/usr/bin/tar",
        nullptr
    };
    
    for (int i = 0; tar_paths[i] != nullptr; i++) {
        if (syscall(SYS_stat, tar_paths[i], &st) == 0) {
            // Verify it's executable
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

struct linux_dirent64 {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

// Check if cron daemon is running with MINIMAL /proc enumeration
// Instead of scanning all PIDs, check known cron PIDs first
static bool check_cron_running_stealthy() {
    // Strategy 1: Check systemd cron service status via /proc/self/cgroup
    // (if running in systemd environment)
    int cgroup_fd = syscall(SYS_open, "/proc/self/cgroup", O_RDONLY);
    if (cgroup_fd >= 0) {
        char cgroup_buf[512];
        long bytes = syscall(SYS_read, cgroup_fd, cgroup_buf, sizeof(cgroup_buf) - 1);
        syscall(SYS_close, cgroup_fd);
        
        if (bytes > 0) {
            cgroup_buf[bytes] = '\0';
            // If we see cron.service in cgroups, cron is likely running
            if (strstr(cgroup_buf, "cron.service")) {
                return true;
            }
        }
    }
    
    // Strategy 2: Check only low PIDs (system daemons typically have low PIDs)
    // This is MUCH stealthier than enumerating all processes
    const int max_pid_check = 500;  // Only check first 500 PIDs
    
    for (int pid = 1; pid <= max_pid_check; pid++) {
        char comm_path[32];
        comm_path[0] = '/';
        comm_path[1] = 'p';
        comm_path[2] = 'r';
        comm_path[3] = 'o';
        comm_path[4] = 'c';
        comm_path[5] = '/';
        
        // Manually convert PID to string
        int pid_len = 0;
        int temp_pid = pid;
        char pid_str[16];
        
        if (temp_pid == 0) {
            pid_str[pid_len++] = '0';
        } else {
            int digits[16];
            int digit_count = 0;
            while (temp_pid > 0) {
                digits[digit_count++] = temp_pid % 10;
                temp_pid /= 10;
            }
            for (int j = digit_count - 1; j >= 0; j--) {
                pid_str[pid_len++] = '0' + digits[j];
            }
        }
        pid_str[pid_len] = '\0';
        
        // Build path: /proc/[pid]/comm
        int path_pos = 6;
        for (int j = 0; j < pid_len; j++) {
            comm_path[path_pos++] = pid_str[j];
        }
        comm_path[path_pos++] = '/';
        comm_path[path_pos++] = 'c';
        comm_path[path_pos++] = 'o';
        comm_path[path_pos++] = 'm';
        comm_path[path_pos++] = 'm';
        comm_path[path_pos] = '\0';
        
        // Try to open and read
        int comm_fd = syscall(SYS_open, comm_path, O_RDONLY);
        if (comm_fd >= 0) {
            char comm_buf[32];
            long bytes = syscall(SYS_read, comm_fd, comm_buf, sizeof(comm_buf) - 1);
            syscall(SYS_close, comm_fd);
            
            if (bytes > 0) {
                comm_buf[bytes] = '\0';
                
                // Check for cron/crond
                if (str_equals(comm_buf, "cron\n") || 
                    str_equals(comm_buf, "crond\n")) {
                    return true;
                }
            }
        }
    }
    
    // Strategy 3: Check if cron socket exists (even stealthier)
    // Many systems use systemd timers or cron sockets
    struct stat st;
    if (syscall(SYS_stat, "/run/crond.pid", &st) == 0) {
        return true;
    }
    if (syscall(SYS_stat, "/var/run/crond.pid", &st) == 0) {
        return true;
    }
    
    return false;
}

// ============================================================================
// STEALTH-ENHANCED USERNAME RETRIEVAL
// ============================================================================

// Get current username (stealthy - uses only stat calls)
static void get_username_stealthy(char* buf, int max_len) {
    // Just return UID as string - no filesystem reads
    unsigned int uid = syscall(SYS_getuid);
    
    // Convert UID to string manually
    if (uid == 0) {
        str_copy(buf, "0", max_len);
        return;
    }
    
    char uid_str[16];
    int pos = 0;
    unsigned int temp_uid = uid;
    
    // Convert to string in reverse
    char temp[16];
    int temp_pos = 0;
    while (temp_uid > 0) {
        temp[temp_pos++] = '0' + (temp_uid % 10);
        temp_uid /= 10;
    }
    
    // Reverse into output buffer
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
    
    // Get current UID (single syscall - unavoidable)
    conditions.current_uid = syscall(SYS_getuid);
    
    // Get username (no filesystem access)
    get_username_stealthy(conditions.username, sizeof(conditions.username));
    
    // Check /tmp/backups directory (single stat call - NO file creation)
    conditions.backups_dir_exists = (syscall(SYS_access, "/tmp/backups", F_OK) == 0);
    conditions.backups_dir_writable = check_directory_writable_stealthy("/tmp/backups");
    
    // Check tar availability (single stat call per path)
    conditions.tar_available = check_tar_available_stealthy();
    
    // Check if cron is running (minimal process checks)
    conditions.cron_running = check_cron_running_stealthy();
    
    return conditions;
}
