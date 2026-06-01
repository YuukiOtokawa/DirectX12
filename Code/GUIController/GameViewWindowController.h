#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
	class GameViewWindowController : public DefaultWindowController {
	public:
		GameViewWindowController() {
			_WindowName = "Game View";
		}
		void Draw() override;
	};
}
