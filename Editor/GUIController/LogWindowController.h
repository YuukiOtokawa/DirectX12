#pragma once
#include "DefaultWindowController.h"

#include "Engine/Source/Utility/Log.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace GUIController::Window {
	class LogWindowController :
		public DefaultWindowController {

	public:
		LogWindowController() {
			m_WindowName = "Log";
		}
		void Update() override;
		void Draw() override;

	private:
		void DrawToolbar(const EngineCore::Log::Entry* selected);
		void DrawList();
		void DrawDetail(const EngineCore::Log::Entry& entry);
		bool IsVisible(EngineCore::Log::Level level) const;

		std::vector<EngineCore::Log::Entry> m_Entries;
		std::vector<int> m_VisibleIndices;
		uint64_t m_LastRevision = 0;
		std::optional<uint64_t> m_SelectedId;
		bool m_ShowInfo = true;
		bool m_ShowWarning = true;
		bool m_ShowError = true;
	};

}
