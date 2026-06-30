#include "Material.h"
#include "RenderManager.h"

namespace Render {

    Material::Material()
        : m_Name("")
        , m_ShaderName("")
        , m_ShaderFilePath("")
        , m_RenderPassType(RenderPassType::DeferredOpaque)
        , m_BaseColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_EmissionColor(0.0f, 0.0f, 0.0f, 1.0f)
        , m_Metallic(0.0f)
        , m_Specular(0.5f)
        , m_Roughness(1.0f)
        , m_NormalWeight(1.0f)
    {
        m_PropertyBuffer.resize(sizeof(MaterialConstant));
        UpdateBufferFromLegacyMembers();
    }

    Material::Material(const std::string& name)
        : m_Name(name)
        , m_ShaderName("")
        , m_ShaderFilePath("")
        , m_RenderPassType(RenderPassType::DeferredOpaque)
        , m_BaseColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_EmissionColor(0.0f, 0.0f, 0.0f, 1.0f)
        , m_Metallic(0.0f)
        , m_Specular(0.5f)
        , m_Roughness(1.0f)
        , m_NormalWeight(1.0f)
    {
        m_PropertyBuffer.resize(sizeof(MaterialConstant));
        UpdateBufferFromLegacyMembers();
    }

    Material::Material(const MaterialConstant& constantData)
        : m_ShaderName("")
        , m_ShaderFilePath("")
        , m_RenderPassType(RenderPassType::DeferredOpaque)
    {
        SetFromConstant(constantData);
    }

    Material::~Material() {}

    void Material::SetBaseColor(const Vector4& color) {
        m_BaseColor = color;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetEmissionColor(const Vector4& color) {
        m_EmissionColor = color;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetMetallic(float metallic) {
        m_Metallic = metallic;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetSpecular(float specular) {
        m_Specular = specular;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetRoughness(float roughness) {
        m_Roughness = roughness;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetNormalWeight(float weight) {
        m_NormalWeight = weight;
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetShader(const std::string& shaderName) {
        m_ShaderName = shaderName;
        auto renderManager = RenderManager::GetInstance();
        if (renderManager) {
            const ShaderMetadata* meta = renderManager->GetShaderMetadata(shaderName);
            if (meta) {
                m_PropertyBuffer.resize(meta->totalSize);
                // Initialize buffer with default values from shader metadata
                for (auto& prop : meta->properties) {
                    if (prop.type == "float4" || prop.type == "float3" || prop.type == "float2") {
                        SetVector(prop.name, prop.defaultValue);
                    } else if (prop.type == "float") {
                        SetFloat(prop.name, prop.defaultValue.x);
                    }
                }
                UpdateLegacyMembersFromBuffer();

                // 動的テクスチャ用のブロックを確保して反映
                if (!meta->textures.empty()) {
                    EnsureTextureBlock();
                    ApplyTextureSlots();
                }
                return;
            }
        }
        // Fallback
        m_PropertyBuffer.resize(sizeof(MaterialConstant));
        UpdateBufferFromLegacyMembers();
    }

    void Material::SetTextureBaseColor(std::shared_ptr<Render::Types::TEXTURE> texture) {
        m_TextureBaseColor = std::move(texture);
    }

    const Render::Types::TEXTURE* Material::GetTextureBaseColor() const {
        return m_TextureBaseColor.get();
    }

    void Material::EnsureTextureBlock() {
        if (m_TextureBlock) return;
        auto rm = RenderManager::GetInstance();
        if (!rm) return;
        unsigned int base = rm->AllocateMaterialTextureBlock();
        m_TextureBlock = std::shared_ptr<unsigned int>(new unsigned int(base), [](unsigned int* p) {
            if (auto rm = RenderManager::GetInstance()) rm->FreeMaterialTextureBlock(*p);
            delete p;
        });
    }

    void Material::ApplyTextureSlots() {
        auto rm = RenderManager::GetInstance();
        if (!rm || !m_TextureBlock || m_ShaderName.empty()) return;
        const ShaderMetadata* meta = rm->GetShaderMetadata(m_ShaderName);
        if (!meta) return;
        for (auto& t : meta->textures) {
            auto it = m_Textures.find(t.name);
            ID3D12Resource* res = (it != m_Textures.end() && it->second) ? it->second->Resource.Get() : nullptr;
            rm->SetMaterialBlockSlot(*m_TextureBlock, t.registerIndex, res); // nullptrはダミーで埋まる
        }
    }

    void Material::SetTexture(const std::string& name, std::shared_ptr<Render::Types::TEXTURE> texture) {
        m_Textures[name] = texture;
        auto rm = RenderManager::GetInstance();
        if (!rm || m_ShaderName.empty()) return;
        const ShaderMetadata* meta = rm->GetShaderMetadata(m_ShaderName);
        if (!meta) return;
        for (auto& t : meta->textures) {
            if (t.name == name) {
                EnsureTextureBlock();
                rm->SetMaterialBlockSlot(*m_TextureBlock, t.registerIndex, texture ? texture->Resource.Get() : nullptr);
                break;
            }
        }
    }

    const Render::Types::TEXTURE* Material::GetTexture(const std::string& name) const {
        auto it = m_Textures.find(name);
        return (it != m_Textures.end()) ? it->second.get() : nullptr;
    }

    void Material::SetFloat(const std::string& name, float value) {
        auto renderManager = RenderManager::GetInstance();
        if (renderManager && !m_ShaderName.empty()) {
            const ShaderMetadata* meta = renderManager->GetShaderMetadata(m_ShaderName);
            if (meta) {
                for (auto& prop : meta->properties) {
                    if (prop.name == name && prop.type == "float") {
                        if (prop.offset + sizeof(float) <= m_PropertyBuffer.size()) {
                            memcpy(m_PropertyBuffer.data() + prop.offset, &value, sizeof(float));
                            UpdateLegacyMembersFromBuffer();
                        }
                        return;
                    }
                }
            }
        }
        
        // Fallback mapping
        if (name == "Metallic") m_Metallic = value;
        else if (name == "Specular") m_Specular = value;
        else if (name == "Roughness") m_Roughness = value;
        else if (name == "NormalWeight") m_NormalWeight = value;
        UpdateBufferFromLegacyMembers();
    }

    float Material::GetFloat(const std::string& name) const {
        auto renderManager = RenderManager::GetInstance();
        if (renderManager && !m_ShaderName.empty()) {
            const ShaderMetadata* meta = renderManager->GetShaderMetadata(m_ShaderName);
            if (meta) {
                for (auto& prop : meta->properties) {
                    if (prop.name == name && prop.type == "float") {
                        if (prop.offset + sizeof(float) <= m_PropertyBuffer.size()) {
                            float val = 0.0f;
                            memcpy(&val, m_PropertyBuffer.data() + prop.offset, sizeof(float));
                            return val;
                        }
                    }
                }
            }
        }
        
        if (name == "Metallic") return m_Metallic;
        if (name == "Specular") return m_Specular;
        if (name == "Roughness") return m_Roughness;
        if (name == "NormalWeight") return m_NormalWeight;
        return 0.0f;
    }

    void Material::SetVector(const std::string& name, const Vector4& value) {
        auto renderManager = RenderManager::GetInstance();
        if (renderManager && !m_ShaderName.empty()) {
            const ShaderMetadata* meta = renderManager->GetShaderMetadata(m_ShaderName);
            if (meta) {
                for (auto& prop : meta->properties) {
                    if (prop.name == name &&
                        (prop.type == "float2" || prop.type == "float3" || prop.type == "float4")) {
                        if (prop.offset + prop.size <= m_PropertyBuffer.size()) {
                            // prop.size 分だけ書き込む（float2/float3 は未使用成分を切り捨て）
                            memcpy(m_PropertyBuffer.data() + prop.offset, &value, prop.size);
                            UpdateLegacyMembersFromBuffer();
                        }
                        return;
                    }
                }
            }
        }

        if (name == "BaseColor") m_BaseColor = value;
        else if (name == "EmissionColor") m_EmissionColor = value;
        UpdateBufferFromLegacyMembers();
    }

    Vector4 Material::GetVector(const std::string& name) const {
        auto renderManager = RenderManager::GetInstance();
        if (renderManager && !m_ShaderName.empty()) {
            const ShaderMetadata* meta = renderManager->GetShaderMetadata(m_ShaderName);
            if (meta) {
                for (auto& prop : meta->properties) {
                    if (prop.name == name &&
                        (prop.type == "float2" || prop.type == "float3" || prop.type == "float4")) {
                        if (prop.offset + prop.size <= m_PropertyBuffer.size()) {
                            Vector4 val(0, 0, 0, 0);
                            memcpy(&val, m_PropertyBuffer.data() + prop.offset, prop.size);
                            return val;
                        }
                    }
                }
            }
        }

        if (name == "BaseColor") return m_BaseColor;
        if (name == "EmissionColor") return m_EmissionColor;
        return Vector4(0, 0, 0, 0);
    }

    void Material::UpdateBufferFromLegacyMembers() {
        if (m_ShaderName.empty()) {
            m_PropertyBuffer.resize(sizeof(MaterialConstant));
            MaterialConstant* data = reinterpret_cast<MaterialConstant*>(m_PropertyBuffer.data());
            data->BaseColor = m_BaseColor;
            data->EmissionColor = m_EmissionColor;
            data->Metallic = m_Metallic;
            data->Specular = m_Specular;
            data->Roughness = m_Roughness;
            data->NormalWeight = m_NormalWeight;
        } else {
            SetVector("BaseColor", m_BaseColor);
            SetVector("EmissionColor", m_EmissionColor);
            SetFloat("Metallic", m_Metallic);
            SetFloat("Specular", m_Specular);
            SetFloat("Roughness", m_Roughness);
            SetFloat("NormalWeight", m_NormalWeight);
        }
    }

    void Material::UpdateLegacyMembersFromBuffer() {
        if (m_ShaderName.empty()) {
            if (m_PropertyBuffer.size() >= sizeof(MaterialConstant)) {
                const MaterialConstant* data = reinterpret_cast<const MaterialConstant*>(m_PropertyBuffer.data());
                m_BaseColor = data->BaseColor;
                m_EmissionColor = data->EmissionColor;
                m_Metallic = data->Metallic;
                m_Specular = data->Specular;
                m_Roughness = data->Roughness;
                m_NormalWeight = data->NormalWeight;
            }
        } else {
            m_BaseColor = GetVector("BaseColor");
            m_EmissionColor = GetVector("EmissionColor");
            m_Metallic = GetFloat("Metallic");
            m_Specular = GetFloat("Specular");
            m_Roughness = GetFloat("Roughness");
            m_NormalWeight = GetFloat("NormalWeight");
        }
    }

    MaterialConstant Material::GetConstantData() const {
        MaterialConstant data;
        data.BaseColor = m_BaseColor;
        data.EmissionColor = m_EmissionColor;
        data.Metallic = m_Metallic;
        data.Specular = m_Specular;
        data.Roughness = m_Roughness;
        data.NormalWeight = m_NormalWeight;
        return data;
    }

    void Material::SetFromConstant(const MaterialConstant& data) {
        m_BaseColor = data.BaseColor;
        m_EmissionColor = data.EmissionColor;
        m_Metallic = data.Metallic;
        m_Specular = data.Specular;
        m_Roughness = data.Roughness;
        m_NormalWeight = data.NormalWeight;
        UpdateBufferFromLegacyMembers();
    }
}
