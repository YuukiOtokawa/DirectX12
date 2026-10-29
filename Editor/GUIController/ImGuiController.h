#pragma once
#include <Windows.h>

#include "ThirdParty/ImGui/Code/imgui.h"

namespace GUIController::Gui {


		class ImGuiController {
		private:
			ImGuiID m_DockspaceID;

			void DrawDockspace();

		public:
			ImGuiController() = default;
			~ImGuiController() = default;
			void Initialize(HWND hWnd);
			void BeginFrame();
			void EndFrame();
			void Finalize();


			LRESULT WindowProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		};

}

namespace GUIController {
	namespace ImGuiControl = Gui;
}

