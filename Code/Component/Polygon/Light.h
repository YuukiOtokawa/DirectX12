#pragma once
#include "Component/Component.h"
#include "Utility/VectorClass.h"

namespace EngineCore::General {

	class Light : public Component {
		REGISTER_COMPONENT(Light)
	private:
		Vector4 m_Color;
		float m_Intensity;
	public:
		Light();
		~Light() override = default;

		void Draw() override;
		void Inspector() override;
		DrawOrder GetDrawOrder() const override { return DrawOrder::Camera; }

		void SetColor(const Vector4& color) { m_Color = color; }
		void SetIntensity(float intensity) { m_Intensity = intensity; }

		Vector4 GetColor() const { return m_Color; }
		float GetIntensity() const { return m_Intensity; }
	};

}
