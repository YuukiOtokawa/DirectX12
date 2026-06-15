#pragma once
#include <string>
#include <vector>
#include "../Utility/VectorClass.h"

namespace Render {
    struct ShaderProperty {
        std::string name;
        std::string type; // "float", "float2", "float3", "float4" など
        size_t offset;
        size_t size;
        Vector4 defaultValue;
    };

    struct ShaderMetadata {
        std::string shaderName;
        std::string shaderFilePath;
        std::vector<ShaderProperty> properties;
        size_t totalSize = 0;
    };

    ShaderMetadata ParseShaderMetadata(const std::string& filePath);
}
