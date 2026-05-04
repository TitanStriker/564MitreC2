#ifndef DOCKER_ENUM_H
#define DOCKER_ENUM_H

#include <string>

struct DetectionResult {
    bool is_docker;
    bool is_vm;
    std::string details;
};

DetectionResult run_environment_checks();

#endif // DOCKER_ENUM_H
