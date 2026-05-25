#pragma once

#include "Object.h"
#include "../Manager/ComponentFactory.h"

namespace EngineCore::General {
	class GameObject;
}

namespace EngineCore::General {

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
		GameObject* _Owner = nullptr;
	public:
		virtual void Start();
		virtual void Update();

		virtual void Draw() {}

		void SetOwner(GameObject* owner) { _Owner = owner; }

		void DrawInspector() override;
		virtual void Inspector();
	protected:
		template <typename T>
		T* GetComponent();

	};

}
#include "../GameObject/GameObject.h"

	template<typename T>
	inline T* EngineCore::General::Component::GetComponent() {
		return _Owner ? _Owner->GetComponent<T>() : nullptr;
	}
