#pragma once
#include "../Utility/VectorClass.h"
#include <string>
#include <vector>
#include <memory>

namespace Render {

    namespace Types {
        struct TEXTURE;
    }

    // シェーダー定数バッファ用およびファイルIO用のPOD構造体
    struct MaterialConstant
    {
        Vector4     BaseColor;
        Vector4     EmissionColor;
        float       Metallic;
        float       Specular;
        float       Roughness;
        float       NormalWeight; // HLSL 側とのアライメント統一用
    };

    class Material
    {
    private:
        std::string m_Name;
        std::string m_ShaderName;

        // 互換用メンバ（同期される）
        Vector4     m_BaseColor;
        Vector4     m_EmissionColor;
        float       m_Metallic;
        float       m_Specular;
        float       m_Roughness;
        float       m_NormalWeight;

        // 動的プロパティバッファ
        std::vector<uint8_t> m_PropertyBuffer;
        std::shared_ptr<Render::Types::TEXTURE> m_TextureBaseColor;

    public:
        Material();
        Material(const std::string& name);
        Material(const MaterialConstant& constantData);
        ~Material();

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        const Vector4& GetBaseColor() const { return m_BaseColor; }
        void SetBaseColor(const Vector4& color);

        const Vector4& GetEmissionColor() const { return m_EmissionColor; }
        void SetEmissionColor(const Vector4& color);

        float GetMetallic() const { return m_Metallic; }
        void SetMetallic(float metallic);

        float GetSpecular() const { return m_Specular; }
        void SetSpecular(float specular);

        float GetRoughness() const { return m_Roughness; }
        void SetRoughness(float roughness);

        float GetNormalWeight() const { return m_NormalWeight; }
        void SetNormalWeight(float weight);

        // 新規追加 API
        void SetShader(const std::string& shaderName);
        const std::string& GetShaderName() const { return m_ShaderName; }

        const void* GetBufferData() const { return m_PropertyBuffer.data(); }
        size_t GetBufferSize() const { return m_PropertyBuffer.size(); }

        void SetTextureBaseColor(std::shared_ptr<Render::Types::TEXTURE> texture);
        const Render::Types::TEXTURE* GetTextureBaseColor() const;

        void SetFloat(const std::string& name, float value);
        float GetFloat(const std::string& name) const;
        void SetVector(const std::string& name, const Vector4& value);
        Vector4 GetVector(const std::string& name) const;

        void UpdateBufferFromLegacyMembers();
        void UpdateLegacyMembersFromBuffer();

        // 定数バッファ用の構造体に変換
        MaterialConstant GetConstantData() const;
        void SetFromConstant(const MaterialConstant& data);
    };
}
