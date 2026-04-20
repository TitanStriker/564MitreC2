#ifndef PRIVESC_CHECK_H
#define PRIVESC_CHECK_H

// Lightweight pre-escalation environment check
struct PrivEscConditions {
    bool backups_dir_exists;
    bool backups_dir_writable;
    bool tar_available;
    bool cron_running;
    unsigned int current_uid;
    char username[32];
};

// Main check function for beachhead
PrivEscConditions check_privesc_viability();

#endif // PRIVESC_CHECK_H
