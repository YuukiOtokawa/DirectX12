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

		bool m_IsStarted = false;

		bool m_IsActive = true;
        std::string m_Name = "object";
    
		std::vector<std::unique_ptr<Component>> m_Components;
	public:

        GameObject();
		~GameObject();

		const std::vector<std::unique_ptr<Component>>& GetComponents() const;
		void ExecUpdate() {
			if (!m_IsStarted) {
				//Start();
				m_IsStarted = true;
			}
			if (m_IsActive) {
				Update();
			}
		}
		virtual void Update();
        void Draw();

		void DrawInspector() override;

		bool IsActive() const { return m_IsActive; }
		void SetActive(bool active) { m_IsActive = active; }
		std::string GetName() const { return m_Name; }
		void SetName(const std::string& name);	

		template<typename T> 
		T* AddComponent();

		template<typename T>
		T* GetComponent();
    };

}

// ==========================================
// Template Inline Implementations
// ==========================================

template<typename T>
inline T* EngineCore::General::GameObject::AddComponent() {
	std::unique_ptr<T> component = std::make_unique<T>();
	component->SetOwner(this);
	m_Components.push_back(std::move(component));
    return dynamic_cast<T *>(m_Components.back().get());
}

template<typename T>
inline T* EngineCore::General::GameObject::GetComponent() {
	for (const auto& component : m_Components) {
		if (auto casted = dynamic_cast<T*>(component.get())) {
			return casted;
		}
	}
	return nullptr;
}

