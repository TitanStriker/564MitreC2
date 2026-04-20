/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Tar Wildcard Privilege Escalation Module
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - Tar wildcard exploitation technique implementation
 * - Direct syscall file operations
 * - Sudoers manipulation logic
 * 
 * Reference: CVE-2016-6321 (tar wildcard injection)
 * For educational use in controlled lab environment only.
 */

#ifndef PRIVESC_H
#define PRIVESC_H

// Execute the tar wildcard privilege escalation exploit
// 
// Creates malicious files in /tmp/backups that will be interpreted
// as tar command-line arguments when the cron job runs:
//   tar -cf /tmp/backup.tar *
// 
// Returns: 0 on success, -1 on failure
int execute_tar_privesc();

// Check if we successfully escalated to root
// Returns: true if UID or EUID is 0
bool check_if_root();

#endif // PRIVESC_H
