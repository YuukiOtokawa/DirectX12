#pragma once

#include "ImGui/Code/imgui.h"
#include "Code/Manager/ObjectManager.h"

#include <optional>
#include <string>
#include <vector>

namespace GUIController::Gui {

// One edit session shared by all hierarchy and inspector windows.
class ObjectNameEditor {
    inline static std::optional<uint64_t> s_ObjectID;
    inline static ImGuiID s_WidgetID = 0;
    inline static std::vector<char> s_Buffer;
    inline static bool s_RequestFocus = false;
    inline static bool s_Hovered = false;
    inline static int s_StartedFrame = -1;
    inline static int s_DrawnFrame = -1;

    static int ResizeBuffer(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            s_Buffer.resize(data->BufSize);
            data->Buf = s_Buffer.data();
        }
        return 0;
    }

    static void Commit() {
        if (!s_ObjectID) {
            return;
        }

        // Resolve the generation-aware ID instead of retaining a raw pointer.
        auto* manager = EngineCore::Manager::ObjectManager::GetInstance();
        for (const auto& entry : manager->GetObjects()) {
            if (entry.object && entry.object->GetID() == *s_ObjectID) {
                entry.object->SetName(s_Buffer.data());
                break;
            }
        }
        s_ObjectID.reset();
        s_WidgetID = 0;
    }

public:
    // Returns true when the displayed name is clicked (also used for selection).
    static bool Draw(EngineCore::General::GameObject& object,
        bool selected = false, bool allowEdit = true) {
        const auto objectKey = std::to_string(object.GetID());
        ImGui::PushID(objectKey.c_str());
        const auto widgetID = ImGui::GetID("##ObjectName");
        bool clicked = false;

        if (s_ObjectID == object.GetID() && s_WidgetID == widgetID) {
            if (s_RequestFocus) {
                ImGui::SetKeyboardFocusHere();
                s_RequestFocus = false;
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            const bool enter = ImGui::InputText(
                "##ObjectNameInput", s_Buffer.data(), s_Buffer.size(),
                ImGuiInputTextFlags_EnterReturnsTrue |
                    ImGuiInputTextFlags_AutoSelectAll |
                    ImGuiInputTextFlags_CallbackResize,
                ResizeBuffer);
            s_Hovered = ImGui::IsItemHovered();
            s_DrawnFrame = ImGui::GetFrameCount();
            if (enter || ImGui::IsItemDeactivated()) {
                Commit();
            }
        }
        else {
            // Draw the name literally, including any ## or ### characters.
            const auto position = ImGui::GetCursorScreenPos();
            clicked = ImGui::Selectable("##ObjectName", selected, 0,
                ImVec2(0.0f, ImGui::GetTextLineHeight()));
            const auto name = object.GetName();
            ImGui::GetWindowDrawList()->AddText(position,
                ImGui::GetColorU32(ImGuiCol_Text), name.c_str());

            if (clicked && allowEdit) {
                Commit();
                const auto currentName = object.GetName();
                s_Buffer.assign(currentName.begin(), currentName.end());
                s_Buffer.push_back('\0');
                s_ObjectID = object.GetID();
                s_WidgetID = widgetID;
                s_RequestFocus = true;
                s_StartedFrame = ImGui::GetFrameCount();
            }
        }

        ImGui::PopID();
        return clicked;
    }

    // Called after all windows: handles blank space, selection changes and
    // hidden/collapsed windows even when the old InputText is no longer drawn.
    static void EndFrame() {
        const auto frame = ImGui::GetFrameCount();
        if (!s_ObjectID || s_StartedFrame == frame) {
            return;
        }
        const bool clickedOutside = !s_Hovered &&
            (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
             ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
             ImGui::IsMouseClicked(ImGuiMouseButton_Middle));
        if (s_DrawnFrame != frame || clickedOutside) {
            Commit();
        }
    }
};

}
