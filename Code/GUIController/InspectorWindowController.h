#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
	class InspectorWindowController :
		public DefaultWindowController {


	public:
		InspectorWindowController() {
			m_WindowName = "Inspector Window";
		}
		void Draw() override;
	};

}

