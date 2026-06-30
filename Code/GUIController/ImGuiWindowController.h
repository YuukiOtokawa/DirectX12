#pragma once

#include "../../ImGui/Code/imgui.h"

#include <cstdint>
#include <string>

namespace GUIController::Gui {

		class ImGuiWindowController {

		protected:
			bool m_IsActive = true;
			std::string m_WindowName;

			uint32_t m_WindowID = 0;

		private:
			static uint32_t m_WindowIDCounter;
		public:
			ImGuiWindowController() : m_WindowName("Window") {
				m_WindowID = m_WindowIDCounter++;
			}
			ImGuiWindowController(const std::string& name) : m_WindowName(name) {
				m_WindowID = m_WindowIDCounter++;
			}
			virtual void Initialize() = 0;
			virtual void Update() = 0;
			void DrawSystem();
			virtual void Draw() = 0;
			virtual void AfterDraw();
			virtual void Finalize() = 0;

			std::string GetName() { return m_WindowName; }
			bool* GetIsActive() { return &m_IsActive; }
		};
}


