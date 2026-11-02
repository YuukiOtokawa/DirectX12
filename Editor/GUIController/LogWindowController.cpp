#include "LogWindowController.h"

using namespace GUIController::Window;
using EngineCore::Log::Entry;
using EngineCore::Log::Level;

namespace {
	const char* ToString(Level level) {
		switch (level) {
		case Level::Info: return "Info";
		case Level::Warning: return "Warning";
		case Level::Error: return "Error";
		case Level::Fatal: return "Fatal";
		}
		return "Unknown";
	}

	// Info は既定の文字色のままにする
	bool GetLevelColor(Level level, ImVec4& color) {
		switch (level) {
		case Level::Warning:
			color = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
			return true;
		case Level::Error:
		case Level::Fatal:
			color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			return true;
		default:
			return false;
		}
	}
}

// タブの裏や折りたたみ中は Draw が呼ばれないので、タイトルはここで更新する
void LogWindowController::Update() {
	const uint64_t errorCount = EngineCore::Log::GetErrorCount();
	if (errorCount == 0) {
		m_WindowName = "Log";
	}
	else {
		m_WindowName = "Log (" + std::to_string(errorCount) + (errorCount == 1 ? " error)" : " errors)");
	}
}

void LogWindowController::Draw() {
	bool filterChanged = false;
	const uint64_t revision = EngineCore::Log::GetRevision();
	if (revision != m_LastRevision) {
		m_Entries = EngineCore::Log::Snapshot();
		m_LastRevision = revision;
		filterChanged = true;
	}

	const Entry* selected = nullptr;
	if (m_SelectedId) {
		for (const Entry& entry : m_Entries) {
			if (entry.id == *m_SelectedId) {
				selected = &entry;
				break;
			}
		}
		// 古い行が捨てられた、または Clear された
		if (!selected) {
			m_SelectedId.reset();
		}
	}

	const bool wasShowInfo = m_ShowInfo;
	const bool wasShowWarning = m_ShowWarning;
	const bool wasShowError = m_ShowError;
	DrawToolbar(selected);
	if (wasShowInfo != m_ShowInfo || wasShowWarning != m_ShowWarning || wasShowError != m_ShowError) {
		filterChanged = true;
	}

	if (filterChanged) {
		m_VisibleIndices.clear();
		for (int i = 0; i < static_cast<int>(m_Entries.size()); ++i) {
			if (IsVisible(m_Entries[i].level)) {
				m_VisibleIndices.push_back(i);
			}
		}
	}

	const float detailHeight = selected ? ImGui::GetContentRegionAvail().y * 0.35f : 0.0f;
	if (ImGui::BeginChild("##LogList", ImVec2(0.0f, -detailHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
		DrawList();
	}
	ImGui::EndChild();

	if (selected) {
		if (ImGui::BeginChild("##LogDetail", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
			DrawDetail(*selected);
		}
		ImGui::EndChild();
	}
}

void LogWindowController::DrawToolbar(const Entry* selected) {
	ImGui::Checkbox("Info", &m_ShowInfo);
	ImGui::SameLine();
	ImGui::Checkbox("Warning", &m_ShowWarning);
	ImGui::SameLine();
	ImGui::Checkbox("Error", &m_ShowError);
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		EngineCore::Log::Clear();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(selected == nullptr);
	if (ImGui::Button("Copy") && selected) {
		ImGui::SetClipboardText(selected->message.c_str());
	}
	ImGui::EndDisabled();
}

void LogWindowController::DrawList() {
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(m_VisibleIndices.size()));
	while (clipper.Step()) {
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
			const Entry& entry = m_Entries[m_VisibleIndices[row]];

			// 行の高さを揃えるため、一覧には本文の最初の行だけを出す
			std::string label = entry.time + " [" + ToString(entry.level) + "] " + entry.message.substr(0, entry.message.find('\n'));
			if (entry.repeatCount > 1) {
				label += " (x" + std::to_string(entry.repeatCount) + ")";
			}
			label += "###LogRow";

			ImVec4 color;
			const bool hasColor = GetLevelColor(entry.level, color);
			if (hasColor) {
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			}
			ImGui::PushID(static_cast<int>(entry.id));
			if (ImGui::Selectable(label.c_str(), m_SelectedId == entry.id)) {
				m_SelectedId = entry.id;
			}
			ImGui::PopID();
			if (hasColor) {
				ImGui::PopStyleColor();
			}
		}
	}
	clipper.End();

	// 末尾を表示中のときだけ追従する
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
}

void LogWindowController::DrawDetail(const Entry& entry) {
	ImGui::PushTextWrapPos(0.0f);
	ImGui::TextUnformatted(entry.message.c_str());
	ImGui::PopTextWrapPos();
}

bool LogWindowController::IsVisible(Level level) const {
	switch (level) {
	case Level::Info: return m_ShowInfo;
	case Level::Warning: return m_ShowWarning;
	default: return m_ShowError;
	}
}
