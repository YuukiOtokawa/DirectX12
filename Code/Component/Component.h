#pragma once

#include "Object.h"
#include "../Manager/ComponentFactory.h"

namespace EngineCore::General {
	class GameObject;
}

namespace EngineCore::General {

	enum class DrawOrder {
		Camera = 0,
		Default = 1,
	};

#define REGISTER_COMPONENT(Type) \
private: \
	static const char* GetClassName() { return #Type; } \
	static inline bool registered = []() { \
		::EngineCore::Manager::ComponentFactory::GetInstance()->RegisterComponent( \
			GetClassName(), \
			[]() -> std::unique_ptr<Component> { return std::make_unique<Type>(); } \
		); \
		return true; \
	 }();

	class Component : public Object {
		REGISTER_COMPONENT(Component)

	protected:
		GameObject* _Owner = nullptr;
	public:
		virtual ~Component() = default;
		virtual void Start();
		virtual void Update();

		virtual void Draw() {}
		virtual DrawOrder GetDrawOrder() const { return DrawOrder::Default; }

		void SetOwner(GameObject* owner) { _Owner = owner; }
		GameObject* GetOwner() const { return _Owner; }

		void DrawInspector() override;
		virtual void Inspector();

	};

}

