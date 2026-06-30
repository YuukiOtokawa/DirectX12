#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
	class GameViewWindowController : public DefaultWindowController {
	public:
		GameViewWindowController() {
			m_WindowName = "Game View";
		}
		void Draw() override;
	};
}
