#include <fstream>
#include <cstdlib>
#include <string>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;


int main() {
    system("echo \"gotcha\" > /tmp/victory");
    return 0;
}
