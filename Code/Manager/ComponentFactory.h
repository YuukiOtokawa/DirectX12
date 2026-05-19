#pragma once

#include <functional>
#include <memory>
#include <map>
#include <string>

namespace EngineCore::General {
	class Component;
}

namespace EngineCore::Manager {
	using namespace EngineCore::General;
	using ComponentCreator = std::function<std::unique_ptr<General::Component>()>;

	class ComponentFactory {

	public: 
		std::string GetComponentTypeName() { return typeid(decltype(this)).name(); } \

		std::map<std::string, ComponentCreator> _creators;
	public:
		static ComponentFactory* GetInstance() {
			static ComponentFactory instance;
			return &instance;
		}

		void RegisterComponent(const std::string& name, ComponentCreator creator) {
			_creators[name] = creator;
		}

		std::unique_ptr<Component> CreateComponent(const std::string& name);

		std::vector<std::string> GetRegisteredComponentNames() const;
	};


}


