#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(YAML_CPP_STATIC_DEFINE)
#define YAML_C_API
#elif defined(yaml_cpp_EXPORTS)
#define YAML_C_API __declspec(dllexport)
#else
#define YAML_C_API __declspec(dllimport)
#endif
#else
#define YAML_C_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct YamlC_Document YamlC_Document;

typedef enum YamlC_Result {
    YAML_C_OK = 0,
    YAML_C_ERROR = 1,
    YAML_C_NOT_FOUND = 2,
    YAML_C_TYPE_MISMATCH = 3,
    YAML_C_BUFFER_TOO_SMALL = 4
} YamlC_Result;

YAML_C_API YamlC_Result yamlc_load_file(const char* file_path, YamlC_Document** out_document);
YAML_C_API void yamlc_destroy_document(YamlC_Document* document);

YAML_C_API YamlC_Result yamlc_get_int32(const YamlC_Document* document, const char* key_path, int32_t* out_value);
YAML_C_API YamlC_Result yamlc_get_float(const YamlC_Document* document, const char* key_path, float* out_value);
YAML_C_API YamlC_Result yamlc_get_bool(const YamlC_Document* document, const char* key_path, int* out_value);
YAML_C_API YamlC_Result yamlc_get_string(
    const YamlC_Document* document,
    const char* key_path,
    char* out_buffer,
    size_t out_buffer_size);

YAML_C_API size_t yamlc_get_last_error(char* out_buffer, size_t out_buffer_size);

#ifdef __cplusplus
}
#endif
