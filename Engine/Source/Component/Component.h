#pragma once

#include "Engine/Public/EngineAPI.h"
#include "Object.h"
#include "Engine/Source/Manager/ComponentFactory.h"

namespace EngineCore::General {
	class GameObject;
}

namespace EngineCore::General {

	enum class DrawOrder {
		Camera = 0,
		Default = 1,
	};

#define REGISTER_COMPONENT(Type) \
public: \
	/* ComponentFactory の登録キーと同じ型名。エディタの Drawer もこの名前で引く */ \
	virtual const char* GetTypeName() const { return #Type; } \
private: \
	static const char* GetClassName() { return #Type; } \
	static inline bool registered = []() { \
		::EngineCore::Manager::ComponentFactory::GetInstance()->RegisterComponent( \
			GetClassName(), \
			[]() -> std::unique_ptr<Component> { return std::make_unique<Type>(); } \
		); \
		return true; \
	 }();

	class ENGINE_API Component : public Object {
		REGISTER_COMPONENT(Component)

	protected:
		GameObject* m_Owner = nullptr;
	public:
		virtual ~Component() = default;
		virtual void Start();
		virtual void Update();

		virtual void Draw() {}
		virtual DrawOrder GetDrawOrder() const { return DrawOrder::Default; }

		void SetOwner(GameObject* owner) { m_Owner = owner; }
		GameObject* GetOwner() const { return m_Owner; }

	};

}
