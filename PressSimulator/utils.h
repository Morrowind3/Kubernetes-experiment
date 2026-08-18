
#ifndef PRESSSIMULATOR_UTILS_H
#define PRESSSIMULATOR_UTILS_H
#include <cstdlib>
#include <string>

template<typename T>
T GetFromEnvOrDefault(const char* envName, T defaultValue) {
    if (const char* value = std::getenv(envName)) {
        try {
            return std::stod(value);
        } catch (std::exception&) {
            return defaultValue;
        }
    }
    return defaultValue;
}


#endif //PRESSSIMULATOR_UTILS_H
