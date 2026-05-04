#ifndef DOCKER_ENUM_H
#define DOCKER_ENUM_H

#include <string>

struct DetectionResult {
    bool is_docker;
    bool is_privileged;
    std::string details;
};

DetectionResult run_environment_checks();

#endif // DOCKER_ENUM_H
