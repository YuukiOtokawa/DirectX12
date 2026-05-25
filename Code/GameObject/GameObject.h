#pragma once
#include "Object.h"

#include <string>
#include <vector>
#include <memory>

namespace EngineCore::General {
	class Component;
}

namespace EngineCore::General {
    class GameObject :
        public Object {

		bool _IsStarted = false;

		bool _IsActive = true;
        std::string _Name = "object";
    
		std::vector<std::unique_ptr<Component>> m_Components;
	public:

        GameObject();
		~GameObject();
		void ExecUpdate() {
			if (!_IsStarted) {
				//Start();
				_IsStarted = true;
			}
			if (_IsActive) {
				Update();
			}
		}
		virtual void Update();
        void Draw();

		void DrawInspector() override;

		bool IsActive() const { return _IsActive; }
		void SetActive(bool active) { _IsActive = active; }
		std::string GetName() const { return _Name; }
		void SetName(const std::string& name);	

		template<typename T>
		void AddComponent();

		// 指定した型のコンポーネントを取得
		template<typename T>
		T* GetComponent();
    };



}

template<typename T>
inline void EngineCore::General::GameObject::AddComponent() {
	// TがComponentの派生クラスであることを確認
	static_assert(std::is_base_of<Component, T>::value, "T must be a subclass of Component");
	std::unique_ptr<T> component = std::make_unique<T>();
	component->_Owner = this;
	m_Components.push_back(std::move(component));
}


template<typename T>
inline T* EngineCore::General::GameObject::GetComponent() {
	// TがComponentの派生クラスであることを確認
	static_assert(std::is_base_of<Component, T>::value, "T must be a subclass of Component");

	for (const auto& component : m_Components) {
		if (auto casted = dynamic_cast<T*>(component.get())) {
			return casted;
		}
	}
	return nullptr;
}