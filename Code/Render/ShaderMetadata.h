#pragma once
#include <string>
#include <vector>
#include "../Utility/VectorClass.h"

namespace EngineCore::Render {
    struct ShaderProperty {
        std::string name;
        std::string type; // "float", "float2", "float3", "float4" など
        size_t offset;
        size_t size;
        Vector4 defaultValue;

        // Unity風アノテーション（HLSLの行末コメントから解析）
        std::string displayName;      // 空なら name を表示に使用
        std::string header;           // 非空ならこのプロパティの前にヘッダを表示
        bool   isColor = false;       // float3/float4 をカラーピッカーで表示
        bool   isHDR = false;         // [HDR] 指定。カラーピッカーの 0..1 クランプを外す（自己発光など）
        bool   hasRange = false;      // [Range(min,max)] が指定された
        float  rangeMin = 0.0f;
        float  rangeMax = 1.0f;
    };

    struct ShaderTextureProperty {
        std::string  name;          // HLSL変数名
        unsigned int registerIndex; // space1内の tN（= マテリアルブロックのスロット番号）
        std::string  displayName;   // 空なら name を表示に使用
        std::string  header;        // 任意（[Header(...)]）
    };

    struct ShaderMetadata {
        std::string shaderName;
        std::string shaderFilePath;
        std::vector<ShaderProperty> properties;
        std::vector<ShaderTextureProperty> textures;  // register space1 のテクスチャ
        size_t totalSize = 0;
    };

    ShaderMetadata ParseShaderMetadata(const std::string& filePath);
}
