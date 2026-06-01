#pragma once
#include "Component/Component.h"
#include "Utility/VectorClass.h"

namespace EngineCore::General {

	class Light : public Component {
		REGISTER_COMPONENT(Light)
	private:
		Vector4 _Color;
		float _Intensity;
	public:
		Light();
		~Light() override = default;

		void Draw() override;
		void Inspector() override;
		DrawOrder GetDrawOrder() const override { return DrawOrder::Camera; }

		void SetColor(const Vector4& color) { _Color = color; }
		void SetIntensity(float intensity) { _Intensity = intensity; }

		Vector4 GetColor() const { return _Color; }
		float GetIntensity() const { return _Intensity; }
	};

}
