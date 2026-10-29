#include "Object.h"

#include "Manager/ObjectManager.h"
using namespace EngineCore::Manager;

bool EngineCore::General::Object::IsValid() const {
	auto manager = ObjectManager::GetInstance();
	return manager->IsValid(GetID());
}
