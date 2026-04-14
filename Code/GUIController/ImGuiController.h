#pragma once
#include <Windows.h>

#include "../../ImGui/Code/imgui.h"

namespace GUIController {
	namespace ImGuiControl {


		class ImGuiController {
		private:
			ImGuiID m_DockspaceID;

			void DrawDockspace();

		public:
			ImGuiController() = default;
			~ImGuiController() = default;
			void Initialize();
			void BeginFrame();
			void EndFrame();
			void Finalize();


			LRESULT WindowProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		};

	}
}

