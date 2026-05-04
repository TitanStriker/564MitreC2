#include "docker_enum.h"
#include <iostream>

int main() {
    std::cout << "Starting environment detection tests..." << std::endl;
    
    DetectionResult result = run_environment_checks();
    
    std::cout << "\n--- Detection Results ---" << std::endl;
    std::cout << "Is Docker:     " << (result.is_docker ? "YES" : "NO") << std::endl;
    std::cout << "Is Privileged: " << (result.is_privileged ? "YES" : "NO") << std::endl;
    std::cout << "\nDetails:\n" << result.details << std::endl;
    std::cout << "-------------------------\n" << std::endl;

    return 0;
}
