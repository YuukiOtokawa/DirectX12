#include "ComponentFactory.h"

#include "Engine/Source/Component/Component.h"

EngineCore::Manager::ComponentFactory* EngineCore::Manager::ComponentFactory::GetInstance() {
	static ComponentFactory instance;
	return &instance;
}

std::unique_ptr<EngineCore::General::Component> EngineCore::Manager::ComponentFactory::CreateComponent(const std::string& name) {
	if (m_Creators.find(name) != m_Creators.end()) {
        return m_Creators[name]();
    }
	return nullptr;
}

std::vector<std::string> EngineCore::Manager::ComponentFactory::GetRegisteredComponentNames() const {
	std::vector<std::string> names;
	for (const auto& pair : m_Creators) {
		names.push_back(pair.first);
	}
	return names;
}
