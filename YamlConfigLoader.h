#pragma once

#include <cstdint>
#include <string>

namespace Config {

struct AppConfig {
    int32_t Width = 1280;
    int32_t Height = 720;
    bool Windowed = true;
};

bool LoadAppConfig(const char* filePath, AppConfig* outConfig, std::string* outError = nullptr);

}

