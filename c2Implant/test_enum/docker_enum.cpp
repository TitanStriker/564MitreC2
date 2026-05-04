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
    if (file_exists("/.dockerenv")) return true;

    const char* cgroup_paths[] = {"/proc/self/cgroup", "/proc/1/cgroup"};
    for (const char* path : cgroup_paths) {
        std::string cgroup = read_file(path);
        if (cgroup.find("docker") != std::string::npos || 
            cgroup.find("kubepods") != std::string::npos ||
            cgroup.find("containerd") != std::string::npos) return true;
    }

    std::string mountinfo = read_file("/proc/self/mountinfo");
    if (mountinfo.find("docker") != std::string::npos) return true;

    return false;
}

static bool check_privileged(std::string& details) {
    bool privileged = false;

    // 1. Check for sensitive device nodes (like /dev/sda)
    // In a normal container, block devices are typically missing.
    if (file_exists("/dev/sda") || file_exists("/dev/vda") || file_exists("/dev/nvme0n1")) {
        privileged = true;
        details += "[!] Sensitive device nodes found in /dev (Potential Privileged Mode). ";
    }

    // 2. Check capabilities via /proc/self/status
    // 0000003fffffffff is often seen in privileged containers (all caps)
    std::string status = read_file("/proc/self/status");
    size_t cap_pos = status.find("CapEff:");
    if (cap_pos != std::string::npos) {
        std::string cap_line = status.substr(cap_pos, status.find("\n", cap_pos) - cap_pos);
        details += "Process " + cap_line + ". ";
        if (cap_line.find("fffffffff") != std::string::npos) {
            privileged = true;
            details += "[!] Full capabilities detected (Privileged). ";
        }
    }

    // 3. Check if we can see all mounts (host mount namespace leak)
    std::string mounts = read_file("/proc/mounts");
    if (mounts.find("ext4") != std::string::npos && mounts.find("nodev") == std::string::npos) {
        // Simple heuristic: many ext4 mounts often indicate host disk visibility
    }

    return privileged;
}

DetectionResult run_environment_checks() {
    DetectionResult result;
    result.is_docker = check_docker();
    result.details = "";
    
    if (result.is_docker) {
        result.details = "Environment: Docker Container. ";
        result.is_privileged = check_privileged(result.details);
        if (result.is_privileged) {
            result.details += "CONTAINER IS PRIVILEGED.";
        } else {
            result.details += "Container is likely unprivileged.";
        }
    } else {
        result.is_docker = false;
        result.is_privileged = false;
        result.details = "Environment: Likely Host/Physical/VM (Non-Docker).";
    }

    return result;
}
