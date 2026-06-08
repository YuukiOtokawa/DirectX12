#include "Material.h"

namespace Render {

    Material::Material()
        : m_Name("")
        , m_BaseColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_EmissionColor(0.0f, 0.0f, 0.0f, 1.0f)
        , m_Metallic(0.0f)
        , m_Specular(0.5f)
        , m_Roughness(1.0f)
        , m_NormalWeight(1.0f)
    {}

    Material::Material(const std::string& name)
        : m_Name(name)
        , m_BaseColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_EmissionColor(0.0f, 0.0f, 0.0f, 1.0f)
        , m_Metallic(0.0f)
        , m_Specular(0.5f)
        , m_Roughness(1.0f)
        , m_NormalWeight(1.0f)
    {}

    Material::Material(const MaterialConstant& constantData)
    {
        SetFromConstant(constantData);
    }

    Material::~Material() {}

    MaterialConstant Material::GetConstantData() const
    {
        MaterialConstant data;
        data.BaseColor = m_BaseColor;
        data.EmissionColor = m_EmissionColor;
        data.Metallic = m_Metallic;
        data.Specular = m_Specular;
        data.Roughness = m_Roughness;
        data.NormalWeight = m_NormalWeight;
        return data;
    }

    void Material::SetFromConstant(const MaterialConstant& data)
    {
        m_BaseColor = data.BaseColor;
        m_EmissionColor = data.EmissionColor;
        m_Metallic = data.Metallic;
        m_Specular = data.Specular;
        m_Roughness = data.Roughness;
        m_NormalWeight = data.NormalWeight;
    }
}
