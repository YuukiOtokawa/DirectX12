#pragma once
#include <string>
#include <vector>
#include "Engine/Source/Utility/VectorClass.h"
#include "Engine/Public/EngineAPI.h"

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

    // .hlsl のテキストを解析してメタデータを作る。
    // Distribution では .hlsl を配布しないので、代わりに ShaderCompiler.exe が書き出した .shadermeta を読む
    ENGINE_API ShaderMetadata ParseShaderMetadata(const std::string& filePath);

    // .shadermeta の読み書き（ShaderCompiler.exe が書き、製品のエンジンが読む）
    ENGINE_API bool SaveShaderMetadata(const std::string& filePath, const ShaderMetadata& metadata);
    ENGINE_API bool LoadShaderMetadata(const std::string& filePath, ShaderMetadata& metadata);
}
