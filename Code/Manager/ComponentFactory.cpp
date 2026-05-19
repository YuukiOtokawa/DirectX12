#include "ComponentFactory.h"

#include "../Component/Component.h"

using namespace EngineCore::Manager;
using namespace EngineCore::General;

std::unique_ptr<Component> EngineCore::Manager::ComponentFactory::CreateComponent(const std::string& name) {
	if (_creators.find(name) != _creators.end()) {
        return _creators[name]();
    }
	return nullptr;
}

std::vector<std::string> EngineCore::Manager::ComponentFactory::GetRegisteredComponentNames() const {
	std::vector<std::string> names;
	for (const auto& pair : _creators) {
		names.push_back(pair.first);
	}
	return names;
}
