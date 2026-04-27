#pragma once
#include "DefaultWindowController.h"

namespace GUIController::Window {
	class InspectorWindowController :
		public DefaultWindowController {


	public:
		InspectorWindowController() {
			_WindowName = "Inspector Window";
		}
		void Draw() override;
	};

}

