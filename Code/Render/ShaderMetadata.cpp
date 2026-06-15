#include "ShaderMetadata.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace Render {

    static size_t GetTypeSize(const std::string& type) {
        if (type == "float") return 4;
        if (type == "float2") return 8;
        if (type == "float3") return 12;
        if (type == "float4") return 16;
        return 0;
    }

    static Vector4 ParseDefaultValue(const std::string& type, std::string valStr) {
        // Trim
        valStr.erase(0, valStr.find_first_not_of(" \t\r\n"));
        valStr.erase(valStr.find_last_not_of(" \t\r\n") + 1);
        if (valStr.empty()) {
            if (type == "float4") return Vector4(1, 1, 1, 1); // Default white/opaque for vector
            return Vector4(0, 0, 0, 0);
        }

        if (type == "float") {
            if (!valStr.empty() && valStr.back() == 'f') valStr.pop_back();
            try {
                float v = std::stof(valStr);
                return Vector4(v, 0, 0, 0);
            } catch (...) {
                return Vector4(0, 0, 0, 0);
            }
        }
        else if (type == "float2" || type == "float3" || type == "float4") {
            std::regex numRegex(R"(-?\d*\.?\d+f?)");
            auto words_begin = std::sregex_iterator(valStr.begin(), valStr.end(), numRegex);
            auto words_end = std::sregex_iterator();

            float values[4] = { 0, 0, 0, 0 };
            int idx = 0;
            for (std::sregex_iterator i = words_begin; i != words_end && idx < 4; ++i) {
                std::string n = i->str();
                if (!n.empty() && n.back() == 'f') n.pop_back();
                try {
                    values[idx++] = std::stof(n);
                } catch (...) {}
            }
            if (type == "float4" && idx < 4) {
                if (idx == 3) values[3] = 1.0f; // Default alpha to 1
            }
            return Vector4(values[0], values[1], values[2], values[3]);
        }
        return Vector4(0, 0, 0, 0);
    }

    ShaderMetadata ParseShaderMetadata(const std::string& filePath) {
        ShaderMetadata metadata;
        metadata.shaderFilePath = filePath;
        
        // Extract shader name from file path
        size_t lastSlash = filePath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            metadata.shaderName = filePath.substr(lastSlash + 1);
        } else {
            metadata.shaderName = filePath;
        }
        size_t lastDot = metadata.shaderName.find_last_of(".");
        if (lastDot != std::string::npos) {
            metadata.shaderName = metadata.shaderName.substr(0, lastDot);
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            // Fallback default metadata (legacy MaterialConstant layout)
            metadata.properties = {
                {"BaseColor", "float4", 0, 16, Vector4(1,1,1,1)},
                {"EmissionColor", "float4", 16, 16, Vector4(0,0,0,1)},
                {"Metallic", "float", 32, 4, Vector4(0,0,0,0)},
                {"Specular", "float", 36, 4, Vector4(0.5f,0,0,0)},
                {"Roughness", "float", 40, 4, Vector4(1,0,0,0)},
                {"NormalWeight", "float", 44, 4, Vector4(1,0,0,0)}
            };
            metadata.totalSize = 48; // 3 * 16 bytes
            return metadata;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // 1. Search for cbuffer
        // Target: cbuffer register(b3) OR with name containing "Material"/"Subset"
        std::regex cbufferRegex(R"(cbuffer\s+(\w+)\s*(?::\s*register\s*\(\s*b3\s*\))?\s*\{([^}]+)\})");
        std::smatch match;
        
        std::string cbufferContent = "";
        std::string searchString = content;
        
        while (std::regex_search(searchString, match, cbufferRegex)) {
            std::string name = match[1].str();
            std::string body = match[2].str();
            
            if (name.find("Material") != std::string::npos || 
                name.find("Subset") != std::string::npos || 
                match[0].str().find("register(b3)") != std::string::npos ||
                match[0].str().find("register( b3 )") != std::string::npos) {
                cbufferContent = body;
                break;
            }
            searchString = match.suffix().str();
        }

        // 2. If cbuffer body not found in shader, check Common.hlsli if included
        if (cbufferContent.empty()) {
            std::string commonPath = "Code/Shader/Common.hlsli";
            if (lastSlash != std::string::npos) {
                commonPath = filePath.substr(0, lastSlash + 1) + "Common.hlsli";
            }
            std::ifstream commonFile(commonPath);
            if (commonFile.is_open()) {
                std::string commonContent((std::istreambuf_iterator<char>(commonFile)), std::istreambuf_iterator<char>());
                commonFile.close();
                searchString = commonContent;
                if (std::regex_search(searchString, match, cbufferRegex)) {
                    cbufferContent = match[2].str();
                }
            }
        }

        // If still empty, use default legacy fallback
        if (cbufferContent.empty()) {
            metadata.properties = {
                {"BaseColor", "float4", 0, 16, Vector4(1,1,1,1)},
                {"EmissionColor", "float4", 16, 16, Vector4(0,0,0,1)},
                {"Metallic", "float", 32, 4, Vector4(0,0,0,0)},
                {"Specular", "float", 36, 4, Vector4(0.5f,0,0,0)},
                {"Roughness", "float", 40, 4, Vector4(1,0,0,0)},
                {"NormalWeight", "float", 44, 4, Vector4(1,0,0,0)}
            };
            metadata.totalSize = 48;
            return metadata;
        }

        // 3. Extract variables from cbuffer content
        std::regex propRegex(R"((float4|float3|float2|float)\s+(\w+)\s*(?:=\s*([^;]+))?\s*;)");
        auto words_begin = std::sregex_iterator(cbufferContent.begin(), cbufferContent.end(), propRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch m = *i;
            ShaderProperty prop;
            prop.type = m[1].str();
            prop.name = m[2].str();
            
            // Skip structural declarations
            if (prop.name == "MATERIAL" || prop.name == "Material") continue;
            
            std::string defValStr = m[3].str();
            prop.size = GetTypeSize(prop.type);
            prop.defaultValue = ParseDefaultValue(prop.type, defValStr);
            prop.offset = 0; 
            
            metadata.properties.push_back(prop);
        }

        // 4. Align variables according to HLSL Cbuffer packing rules
        size_t curr_offset = 0;
        for (auto& prop : metadata.properties) {
            size_t size = prop.size;
            // Check if variable crosses a 16-byte boundary
            if (curr_offset / 16 != (curr_offset + size - 1) / 16) {
                // Align to next 16-byte boundary
                curr_offset = ((curr_offset + 15) / 16) * 16;
            }
            prop.offset = curr_offset;
            curr_offset += size;
        }
        metadata.totalSize = ((curr_offset + 15) / 16) * 16;

        return metadata;
    }
}
