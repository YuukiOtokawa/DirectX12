#pragma once

// コンポーネントの型名 → Inspector の描画関数のレジストリ（Unity の CustomEditor 相当）
// コンポーネント側は UI を持たず、エディタ側の Drawer が公開 API 越しに値を編集する

#include <functional>
#include <map>
#include <string>

namespace EngineCore::General {
	class Component;
}

namespace EditorUI {

	using ComponentDrawer = std::function<void(EngineCore::General::Component&)>;

	class InspectorRegistry {
		std::map<std::string, ComponentDrawer> m_Drawers;
	public:
		static InspectorRegistry& Get();

		void Register(const std::string& typeName, ComponentDrawer drawer);

		// 型名の見出しと、登録された Drawer を描く。登録が無ければその旨だけ描く
		void Draw(EngineCore::General::Component& component);

		// 指定した型名の Drawer だけを描く（派生コンポーネントの Drawer から基底の Drawer を呼ぶ用）
		bool DrawAs(const std::string& typeName, EngineCore::General::Component& component);
	};
}

// Drawer の .cpp に書く。静的初期化でレジストリに登録される
#define REGISTER_INSPECTOR(Type, Fn) \
	static const bool s_InspectorRegistered_##Type = [] { \
		::EditorUI::InspectorRegistry::Get().Register(#Type, Fn); \
		return true; \
	}();
