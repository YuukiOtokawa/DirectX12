#pragma once
#include "../Utility/VectorClass.h"
#include <string>

namespace Render {

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
        Vector4     m_BaseColor;
        Vector4     m_EmissionColor;
        float       m_Metallic;
        float       m_Specular;
        float       m_Roughness;
        float       m_NormalWeight;

    public:
        Material();
        Material(const std::string& name);
        Material(const MaterialConstant& constantData);
        ~Material();

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        const Vector4& GetBaseColor() const { return m_BaseColor; }
        void SetBaseColor(const Vector4& color) { m_BaseColor = color; }

        const Vector4& GetEmissionColor() const { return m_EmissionColor; }
        void SetEmissionColor(const Vector4& color) { m_EmissionColor = color; }

        float GetMetallic() const { return m_Metallic; }
        void SetMetallic(float metallic) { m_Metallic = metallic; }

        float GetSpecular() const { return m_Specular; }
        void SetSpecular(float specular) { m_Specular = specular; }

        float GetRoughness() const { return m_Roughness; }
        void SetRoughness(float roughness) { m_Roughness = roughness; }

        float GetNormalWeight() const { return m_NormalWeight; }
        void SetNormalWeight(float weight) { m_NormalWeight = weight; }

        // 定数バッファ用の構造体に変換
        MaterialConstant GetConstantData() const;
        void SetFromConstant(const MaterialConstant& data);
    };
}
