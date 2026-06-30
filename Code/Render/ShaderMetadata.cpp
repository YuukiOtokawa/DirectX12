#include "ShaderMetadata.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace EngineCore::Render {

    static size_t GetTypeSize(const std::string& type) {
        if (type == "float") return 4;
        if (type == "float2") return 8;
        if (type == "float3") return 12;
        if (type == "float4") return 16;
        return 0;
    }

    static Vector4 ParseDefaultValue(const std::string& type, std::string valStr); // 前方宣言

    // "1.0f" や " 0.5 " のような文字列を float に変換（f サフィックス・空白を許容）
    static float ParseFloatToken(std::string s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        if (!s.empty() && (s.back() == 'f' || s.back() == 'F')) s.pop_back();
        try { return std::stof(s); } catch (...) { return 0.0f; }
    }

    // 行末コメント（例: " [Header(Surface)] [Range(0,1)] \"Base Color\" "）から
    // Unity風アノテーションを解析して prop に反映する
    static void ParsePropertyAttributes(const std::string& comment, ShaderProperty& prop) {
        if (comment.empty()) return;

        // 表示名: ダブルクォートで囲まれた文字列
        std::smatch m;
        std::regex nameRegex("\"([^\"]*)\"");
        if (std::regex_search(comment, m, nameRegex)) {
            prop.displayName = m[1].str();
        }

        // [Range(min,max)]
        std::regex rangeRegex(R"(\[\s*[Rr]ange\s*\(\s*([^,\)]+)\s*,\s*([^\)]+)\s*\)\s*\])");
        if (std::regex_search(comment, m, rangeRegex)) {
            prop.hasRange = true;
            prop.rangeMin = ParseFloatToken(m[1].str());
            prop.rangeMax = ParseFloatToken(m[2].str());
        }

        // [Header(Foo)]
        std::regex headerRegex(R"(\[\s*[Hh]eader\s*\(\s*([^\)]*?)\s*\)\s*\])");
        if (std::regex_search(comment, m, headerRegex)) {
            prop.header = m[1].str();
        }

        // [Default(...)]: HLSLのcbufferメンバは初期化子を書けないため、
        // コメントでデフォルト値を指定できるようにする
        std::regex defaultRegex(R"(\[\s*[Dd]efault\s*\(([^\)]*)\)\s*\])");
        if (std::regex_search(comment, m, defaultRegex)) {
            prop.defaultValue = ParseDefaultValue(prop.type, m[1].str());
        }

        // [Color] / [Vector]（明示指定はデフォルトを上書き）
        if (std::regex_search(comment, std::regex(R"(\[\s*[Cc]olor\s*\])"))) {
            prop.isColor = true;
        }
        if (std::regex_search(comment, std::regex(R"(\[\s*[Vv]ector\s*\])"))) {
            prop.isColor = false;
        }
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
        //    末尾の行末コメント（// ...）も取り込み、アノテーション解析に使う
        std::regex propRegex(R"((float4|float3|float2|float)\s+(\w+)\s*(?:=\s*([^;]+?))?\s*;[ \t]*(//[^\r\n]*)?)");
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

            // アノテーション無し時のデフォルト色判定:
            // float4 は従来通りカラー、float2/float3 はベクトル
            prop.isColor = (prop.type == "float4");

            // 行末コメントから Unity風アノテーションを反映
            ParsePropertyAttributes(m[4].str(), prop);

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

        // 5. テクスチャプロパティ（register space1 のみ。space0はエンジン予約なので対象外）
        {
            std::regex texRegex(R"(Texture2D(?:<[^>]*>)?\s+(\w+)\s*:\s*register\s*\(\s*t(\d+)\s*,\s*space(\d+)\s*\)\s*;[ \t]*(//[^\r\n]*)?)");
            auto tb = std::sregex_iterator(content.begin(), content.end(), texRegex);
            auto te = std::sregex_iterator();
            for (std::sregex_iterator i = tb; i != te; ++i) {
                std::smatch tm = *i;
                unsigned int space = (unsigned int)std::stoul(tm[3].str());
                if (space == 0) continue; // エンジン予約(space0)は除外

                ShaderTextureProperty tp;
                tp.name = tm[1].str();
                tp.registerIndex = (unsigned int)std::stoul(tm[2].str());

                std::string comment = tm[4].str();
                std::smatch cm;
                if (std::regex_search(comment, cm, std::regex("\"([^\"]*)\"")))
                    tp.displayName = cm[1].str();
                if (std::regex_search(comment, cm, std::regex(R"(\[\s*[Hh]eader\s*\(\s*([^\)]*?)\s*\)\s*\])")))
                    tp.header = cm[1].str();

                metadata.textures.push_back(tp);
            }
        }

        return metadata;
    }
}
