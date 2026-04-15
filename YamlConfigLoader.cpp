#include "YamlConfigLoader.h"

#include <array>

#include "c-api/yaml_c_api.h"

namespace Config {

namespace {
std::string ReadLastError() {
    constexpr size_t kBufferSize = 1024;
    std::array<char, kBufferSize> buffer{};
    const size_t required = yamlc_get_last_error(buffer.data(), buffer.size());
    if (required == 0) {
        return "Unknown yaml-cpp c-api error.";
    }
    if (required <= buffer.size()) {
        return std::string(buffer.data());
    }

    std::string dynamicBuffer(required, '\0');
    yamlc_get_last_error(dynamicBuffer.data(), dynamicBuffer.size());
    return std::string(dynamicBuffer.c_str());
}

void AssignError(std::string* outError, const char* fallback) {
    if (!outError) {
        return;
    }
    std::string error = ReadLastError();
    if (error.empty()) {
        error = fallback;
    }
    *outError = error;
}
}

bool LoadAppConfig(const char* filePath, AppConfig* outConfig, std::string* outError) {
    if (outConfig == nullptr || filePath == nullptr || filePath[0] == '\0') {
        if (outError) {
            *outError = "LoadAppConfig: invalid argument.";
        }
        return false;
    }

    YamlC_Document* document = nullptr;
    if (yamlc_load_file(filePath, &document) != YAML_C_OK) {
        AssignError(outError, "Failed to load YAML file.");
        return false;
    }

    AppConfig loaded = *outConfig;

    int32_t width = 0;
    if (yamlc_get_int32(document, "window.width", &width) == YAML_C_OK) {
        loaded.Width = width;
    }

    int32_t height = 0;
    if (yamlc_get_int32(document, "window.height", &height) == YAML_C_OK) {
        loaded.Height = height;
    }

    int windowed = 1;
    if (yamlc_get_bool(document, "window.windowed", &windowed) == YAML_C_OK) {
        loaded.Windowed = (windowed != 0);
    }

    yamlc_destroy_document(document);
    *outConfig = loaded;
    if (outError) {
        outError->clear();
    }
    return true;
}

}

