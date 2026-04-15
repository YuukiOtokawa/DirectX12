#include "../c-api/yaml_c_api.h"

#include <cstring>
#include <exception>
#include <string>

#include "../include/yaml-cpp/yaml.h"

struct YamlC_Document {
    YAML::Node root;
};

namespace {
thread_local std::string g_lastError;

void set_last_error(const std::string& error_text) {
    g_lastError = error_text;
}

YAML::Node resolve_path(const YAML::Node& root, const char* keyPath) {
    if (keyPath == nullptr || keyPath[0] == '\0') {
        return root;
    }

    YAML::Node current = root;
    const std::string path(keyPath);

    size_t start = 0;
    while (start < path.size()) {
        const size_t dot = path.find('.', start);
        const size_t length = (dot == std::string::npos) ? (path.size() - start) : (dot - start);
        if (length == 0) {
            return YAML::Node();
        }

        const std::string key = path.substr(start, length);
        current = current[key];
        if (!current) {
            return YAML::Node();
        }

        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }

    return current;
}
}  // namespace

extern "C" {

YamlC_Result yamlc_load_file(const char* file_path, YamlC_Document** out_document) {
    if (out_document == nullptr || file_path == nullptr || file_path[0] == '\0') {
        set_last_error("yamlc_load_file: invalid argument.");
        return YAML_C_ERROR;
    }

    try {
        YamlC_Document* document = new YamlC_Document{};
        document->root = YAML::LoadFile(file_path);
        *out_document = document;
        set_last_error("");
        return YAML_C_OK;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        *out_document = nullptr;
        return YAML_C_ERROR;
    } catch (...) {
        set_last_error("Unknown error while loading YAML file.");
        *out_document = nullptr;
        return YAML_C_ERROR;
    }
}

void yamlc_destroy_document(YamlC_Document* document) {
    delete document;
}

YamlC_Result yamlc_get_int32(const YamlC_Document* document, const char* key_path, int32_t* out_value) {
    if (document == nullptr || out_value == nullptr) {
        set_last_error("yamlc_get_int32: invalid argument.");
        return YAML_C_ERROR;
    }

    try {
        const YAML::Node node = resolve_path(document->root, key_path);
        if (!node) {
            return YAML_C_NOT_FOUND;
        }
        *out_value = node.as<int32_t>();
        set_last_error("");
        return YAML_C_OK;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return YAML_C_TYPE_MISMATCH;
    }
}

YamlC_Result yamlc_get_float(const YamlC_Document* document, const char* key_path, float* out_value) {
    if (document == nullptr || out_value == nullptr) {
        set_last_error("yamlc_get_float: invalid argument.");
        return YAML_C_ERROR;
    }

    try {
        const YAML::Node node = resolve_path(document->root, key_path);
        if (!node) {
            return YAML_C_NOT_FOUND;
        }
        *out_value = node.as<float>();
        set_last_error("");
        return YAML_C_OK;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return YAML_C_TYPE_MISMATCH;
    }
}

YamlC_Result yamlc_get_bool(const YamlC_Document* document, const char* key_path, int* out_value) {
    if (document == nullptr || out_value == nullptr) {
        set_last_error("yamlc_get_bool: invalid argument.");
        return YAML_C_ERROR;
    }

    try {
        const YAML::Node node = resolve_path(document->root, key_path);
        if (!node) {
            return YAML_C_NOT_FOUND;
        }
        *out_value = node.as<bool>() ? 1 : 0;
        set_last_error("");
        return YAML_C_OK;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return YAML_C_TYPE_MISMATCH;
    }
}

YamlC_Result yamlc_get_string(
    const YamlC_Document* document,
    const char* key_path,
    char* out_buffer,
    size_t out_buffer_size) {
    if (document == nullptr || out_buffer == nullptr || out_buffer_size == 0) {
        set_last_error("yamlc_get_string: invalid argument.");
        return YAML_C_ERROR;
    }

    try {
        const YAML::Node node = resolve_path(document->root, key_path);
        if (!node) {
            return YAML_C_NOT_FOUND;
        }

        const std::string value = node.as<std::string>();
        const size_t required = value.size() + 1;
        if (required > out_buffer_size) {
            set_last_error("Output buffer is too small.");
            return YAML_C_BUFFER_TOO_SMALL;
        }

        std::memcpy(out_buffer, value.c_str(), required);
        set_last_error("");
        return YAML_C_OK;
    } catch (const std::exception& e) {
        set_last_error(e.what());
        return YAML_C_TYPE_MISMATCH;
    }
}

size_t yamlc_get_last_error(char* out_buffer, size_t out_buffer_size) {
    const size_t required = g_lastError.size() + 1;
    if (out_buffer == nullptr || out_buffer_size == 0) {
        return required;
    }
    if (required > out_buffer_size) {
        return required;
    }

    std::memcpy(out_buffer, g_lastError.c_str(), required);
    return required;
}

}  // extern "C"

