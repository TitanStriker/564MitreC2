#include "docker_enum.h"
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>
#include <algorithm>

static bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

static std::string read_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "";
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static bool check_docker() {
    // 1. Check for .dockerenv file
    if (file_exists("/.dockerenv")) return true;

    // 2. Check /proc/1/cgroup
    std::string cgroup = read_file("/proc/1/cgroup");
    if (cgroup.find("docker") != std::string::npos || 
        cgroup.find("kubepods") != std::string::npos) return true;

    // 3. Check /proc/self/mountinfo
    std::string mountinfo = read_file("/proc/self/mountinfo");
    if (mountinfo.find("docker") != std::string::npos) return true;

    return false;
}

static std::string detect_vm_vendor() {
    std::string details = "";

    // 1. Check CPUID hypervisor bit and vendor string
    #if defined(__x86_64__) || defined(__i386__)
    unsigned int eax, ebx, ecx, edx;
    eax = 1;
    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));
    if (ecx & (1u << 31)) {
        eax = 0x40000000;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));
        char vendor[13];
        *((unsigned int*)&vendor[0]) = ebx;
        *((unsigned int*)&vendor[4]) = ecx;
        *((unsigned int*)&vendor[8]) = edx;
        vendor[12] = '\0';
        details += "Hypervisor detected via CPUID: ";
        details += vendor;
        details += ". ";
    }
    #endif

    // 2. Check DMI product name
    std::string product_name = read_file("/sys/class/dmi/id/product_name");
    std::transform(product_name.begin(), product_name.end(), product_name.begin(), ::tolower);
    if (!product_name.empty()) {
        if (product_name.find("vmware") != std::string::npos ||
            product_name.find("virtualbox") != std::string::npos ||
            product_name.find("qemu") != std::string::npos ||
            product_name.find("kvm") != std::string::npos ||
            product_name.find("xen") != std::string::npos) {
            details += "VM detected via DMI product name: " + product_name + ". ";
        }
    }

    // 3. Check /proc/scsi/scsi
    std::string scsi = read_file("/proc/scsi/scsi");
    std::transform(scsi.begin(), scsi.end(), scsi.begin(), ::tolower);
    if (scsi.find("vmware") != std::string::npos ||
        scsi.find("vbox") != std::string::npos) {
        details += "VM detected via SCSI devices. ";
    }

    return details;
}

DetectionResult run_environment_checks() {
    DetectionResult result;
    result.is_docker = check_docker();
    result.details = detect_vm_vendor();
    result.is_vm = !result.details.empty();

    if (result.is_docker) {
        result.details = "Docker/Container environment detected. " + result.details;
    } else if (!result.is_vm) {
        result.details = "Physical machine likely. " + result.details;
    }

    return result;
}
