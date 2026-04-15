#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include <utility>

#include "ImGuiWindowController.h"
using namespace GUIController::ImGuiControl;

namespace GUIController {
	namespace EngineWindow {
		namespace WindowManager {

			class WindowManager {
				std::vector<std::shared_ptr<ImGuiWindowController>> _Windows;

			public:
				template<class T, class... Args>
				T* NewWindow(Args&&... args);

				void Initialize();
				void Draw();
				void DrawWindows();
				void Finalize();
			};


			template<class T, class... Args>
			inline T* WindowManager::NewWindow(Args&&... args) {
				static_assert(std::is_base_of<ImGuiWindowController, T>::value,
					"T must derive from ImGuiWindowController");

				auto newWindow = std::make_shared<T>(std::forward<Args>(args)...);
				_Windows.push_back(newWindow);
				return newWindow.get();
			}

		}
	}
}

