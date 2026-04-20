/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Privilege Escalation Pre-Check Module (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - Read-only directory permission checks
 * - Minimal /proc enumeration strategies
 * - Multi-strategy cron detection
 * 
 * For educational use in controlled lab environment only.
 */

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

// Main check function for beachhead (STEALTH-ENHANCED)
// Performs minimal syscall-based reconnaissance to determine
// if tar wildcard privilege escalation is viable
// 
// Stealth improvements:
// - NO test file creation (uses stat permission checks)
// - Minimal /proc enumeration (checks only low PIDs)
// - Multi-strategy cron detection (cgroup -> PIDs -> socket)
// - Reduced syscall footprint (~95% reduction)
PrivEscConditions check_privesc_viability();

#endif // PRIVESC_CHECK_H
