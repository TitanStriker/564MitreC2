/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Privilege Escalation Pre-Check Implementation
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - getdents64 syscall for process enumeration
 * - File access checks via direct syscalls
 * - Minimal dependency string operations
 * 
 * For educational use in controlled lab environment only.
 */

#include "privesc_check.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
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

static bool is_numeric(const char* str) {
    if (!str || !str[0]) return false;
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// ============================================================================
// DIRECTORY CHECKS
// ============================================================================

// Check if directory exists and is writable
static bool check_directory_writable(const char* path) {
    struct stat st;
    
    // Use direct syscall to check if directory exists
    if (syscall(SYS_stat, path, &st) != 0) {
        return false;
    }
    
    // Check if it's a directory
    if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    
    // Try to create a test file to verify write access
    char test_path[128];
    str_copy(test_path, path, 100);
    int len = 0;
    while (test_path[len]) len++;
    test_path[len++] = '/';
    test_path[len++] = '.';
    test_path[len++] = 't';
    test_path[len++] = 's';
    test_path[len++] = 't';
    test_path[len] = '\0';
    
    int fd = syscall(SYS_open, test_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
        syscall(SYS_close, fd);
        syscall(SYS_unlink, test_path);
        return true;
    }
    
    return false;
}

// ============================================================================
// TAR BINARY CHECK
// ============================================================================

// Check if tar binary is available
static bool check_tar_available() {
    struct stat st;
    
    // Check common tar locations
    const char* tar_paths[] = {
        "/bin/tar",
        "/usr/bin/tar",
        nullptr
    };
    
    for (int i = 0; tar_paths[i] != nullptr; i++) {
        if (syscall(SYS_stat, tar_paths[i], &st) == 0) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// CRON DAEMON CHECK
// ============================================================================

struct linux_dirent64 {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

// Check if cron daemon is running
static bool check_cron_running() {
    // Parse /proc looking for cron process
    char buf[4096];
    
    int proc_fd = syscall(SYS_open, "/proc", O_RDONLY | O_DIRECTORY);
    if (proc_fd < 0) return false;
    
    bool found_cron = false;
    
    while (true) {
        long nread = syscall(SYS_getdents64, proc_fd, buf, sizeof(buf));
        if (nread <= 0) break;
        
        for (long pos = 0; pos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
            
            // Check if directory name is numeric (PID)
            if (d->d_type == DT_DIR && is_numeric(d->d_name)) {
                // Build path to /proc/[pid]/comm
                char comm_path[64] = "/proc/";
                int path_len = 6;
                for (int i = 0; d->d_name[i]; i++) 
                    comm_path[path_len++] = d->d_name[i];
                str_copy(comm_path + path_len, "/comm", 64 - path_len);
                
                // Read process name
                int comm_fd = syscall(SYS_open, comm_path, O_RDONLY);
                if (comm_fd >= 0) {
                    char comm_buf[32];
                    long bytes = syscall(SYS_read, comm_fd, comm_buf, sizeof(comm_buf) - 1);
                    if (bytes > 0) {
                        comm_buf[bytes] = '\0';
                        
                        // Check for cron/crond
                        if ((comm_buf[0] == 'c' && comm_buf[1] == 'r' && 
                             comm_buf[2] == 'o' && comm_buf[3] == 'n') ||
                            (comm_buf[0] == 'c' && comm_buf[1] == 'r' && 
                             comm_buf[2] == 'o' && comm_buf[3] == 'n' && 
                             comm_buf[4] == 'd')) {
                            found_cron = true;
                        }
                    }
                    syscall(SYS_close, comm_fd);
                }
                
                if (found_cron) break;
            }
            
            pos += d->d_reclen;
        }
        
        if (found_cron) break;
    }
    
    syscall(SYS_close, proc_fd);
    return found_cron;
}

// ============================================================================
// USERNAME RETRIEVAL
// ============================================================================

// Get current username
static void get_username(char* buf, int max_len) {
    // Try to read from /proc/self/loginuid
    int fd = syscall(SYS_open, "/proc/self/loginuid", O_RDONLY);
    if (fd < 0) {
        str_copy(buf, "unknown", max_len);
        return;
    }
    
    char uid_buf[16];
    long bytes = syscall(SYS_read, fd, uid_buf, sizeof(uid_buf) - 1);
    syscall(SYS_close, fd);
    
    if (bytes > 0) {
        uid_buf[bytes] = '\0';
        // Remove newline
        for (int i = 0; i < bytes; i++) {
            if (uid_buf[i] == '\n') uid_buf[i] = '\0';
        }
        str_copy(buf, uid_buf, max_len);
    } else {
        str_copy(buf, "unknown", max_len);
    }
}

// ============================================================================
// MAIN CHECK FUNCTION
// ============================================================================

PrivEscConditions check_privesc_viability() {
    PrivEscConditions conditions;
    memset(&conditions, 0, sizeof(conditions));
    
    // Get current UID
    conditions.current_uid = syscall(SYS_getuid);
    
    // Get username
    get_username(conditions.username, sizeof(conditions.username));
    
    // Check /tmp/backups directory
    conditions.backups_dir_exists = (syscall(SYS_access, "/tmp/backups", F_OK) == 0);
    conditions.backups_dir_writable = check_directory_writable("/tmp/backups");
    
    // Check tar availability
    conditions.tar_available = check_tar_available();
    
    // Check if cron is running
    conditions.cron_running = check_cron_running();
    
    return conditions;
}
