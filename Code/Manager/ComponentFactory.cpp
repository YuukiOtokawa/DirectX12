#include "ComponentFactory.h"

#include "../Component/Component.h"


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
