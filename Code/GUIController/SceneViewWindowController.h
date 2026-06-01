#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
	class SceneViewWindowController : public DefaultWindowController {
	public:
		SceneViewWindowController() {
			_WindowName = "Scene View";
		}
		void Draw() override;
	};
}
