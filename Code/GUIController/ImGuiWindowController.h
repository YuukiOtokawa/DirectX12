#pragma once

#include "../../ImGui/Code/imgui.h"

#include <cstdint>
#include <string>

namespace GUIController::Gui {

		class ImGuiWindowController {
			bool _isActive = true;
			std::string _WindowName;

			uint32_t _WindowID = 0;
			static uint32_t _WindowIDCounter;
		public:
			ImGuiWindowController() : _WindowName("Window") {
				_WindowID = _WindowIDCounter++;
			}
			ImGuiWindowController(const std::string& name) : _WindowName(name) {
				_WindowID = _WindowIDCounter++;
			}
			virtual void Initialize() = 0;
			virtual void Update() = 0;
			void DrawSystem();
			virtual void Draw() = 0;
			virtual void AfterDraw();
			virtual void Finalize() = 0;

			std::string GetName() { return _WindowName; }
			bool* GetIsActive() { return &_isActive; }
		};
}


