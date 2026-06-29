#pragma once
// シェーダメタデータ（HLSLのcbuffer + Unity風アノテーション）から
// マテリアルプロパティのImGui UIを描画する共有ヘルパー。
// MeshRenderer と PostProcessComponent の両方から利用する。
#include "../../ImGui/Code/imgui.h"
#include "Material.h"
#include "ShaderMetadata.h"

namespace GUIHelper {

    // meta に基づき mat の動的プロパティを編集UIとして描画する。
    // いずれかの値が変更されたら true を返す。
    inline bool DrawMaterialProperties(Render::Material& mat, const Render::ShaderMetadata* meta) {
        if (!meta) return false;

        bool changed = false;
        for (auto& prop : meta->properties) {
            // ラベルは表示名優先（無ければ変数名）
            const char* label = prop.displayName.empty() ? prop.name.c_str() : prop.displayName.c_str();

            // [Header(...)] が指定されていれば区切りを出す
            if (!prop.header.empty()) {
                ImGui::SeparatorText(prop.header.c_str());
            }

            if (prop.type == "float4") {
                Vector4 val = mat.GetVector(prop.name);
                float v[4] = { val.x, val.y, val.z, val.w };
                bool edited = prop.isColor
                    ? ImGui::ColorEdit4(label, v)
                    : ImGui::DragFloat4(label, v, 0.01f);
                if (edited) {
                    mat.SetVector(prop.name, Vector4(v[0], v[1], v[2], v[3]));
                    changed = true;
                }
            }
            else if (prop.type == "float3") {
                Vector4 val = mat.GetVector(prop.name);
                float v[3] = { val.x, val.y, val.z };
                bool edited = prop.isColor
                    ? ImGui::ColorEdit3(label, v)
                    : ImGui::DragFloat3(label, v, 0.01f);
                if (edited) {
                    mat.SetVector(prop.name, Vector4(v[0], v[1], v[2], 0.0f));
                    changed = true;
                }
            }
            else if (prop.type == "float2") {
                Vector4 val = mat.GetVector(prop.name);
                float v[2] = { val.x, val.y };
                if (ImGui::DragFloat2(label, v, 0.01f)) {
                    mat.SetVector(prop.name, Vector4(v[0], v[1], 0.0f, 0.0f));
                    changed = true;
                }
            }
            else if (prop.type == "float") {
                float val = mat.GetFloat(prop.name);
                // [Range(min,max)] があればスライダー、無ければドラッグ
                if (prop.hasRange) {
                    if (ImGui::SliderFloat(label, &val, prop.rangeMin, prop.rangeMax)) {
                        mat.SetFloat(prop.name, val);
                        changed = true;
                    }
                } else {
                    if (ImGui::DragFloat(label, &val, 0.01f)) {
                        mat.SetFloat(prop.name, val);
                        changed = true;
                    }
                }
            }
        }
        return changed;
    }

}
