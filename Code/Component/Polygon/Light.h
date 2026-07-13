#pragma once
#include "Component/Component.h"
#include "Utility/VectorClass.h"
#include <DirectXMath.h>
using namespace DirectX;

namespace EngineCore::General {

	class Light : public Component {
		REGISTER_COMPONENT(Light)
	public:
		enum class LightType { Directional, Point, Spot };

		// シャドウカスケード数（RenderStructure::SHADOW_CASCADE_COUNTと一致させること）
		static const int CASCADE_COUNT = 3;

	private:
		// 各カスケードの正射影幅の倍率（近→遠）
		static constexpr float CASCADE_SCALE[CASCADE_COUNT] = { 1.0f, 4.0f, 16.0f };

		// シャドウパスで今描いているカスケード番号（GameManagerが設定する）
		int m_CurrentCascade = 0;

		Vector4 m_Color;
		float m_Exposure;

		LightType m_LightType = LightType::Directional;

		XMMATRIX m_ViewMatrix;
		XMMATRIX m_ProjectionMatrix;

		// シャドウ用パラメータ（Directionalは正射影、Point/Spotは透視投影）
		float m_ShadowOrthoSize;
		float m_ShadowFov;
		float m_ShadowNear;
		float m_ShadowFar;

		static Light* s_ActiveLight;
	public:
		Light();
		~Light() override = default;

		void Update() override;
		void Draw() override;
		void Inspector() override;
		DrawOrder GetDrawOrder() const override { return DrawOrder::Camera; }

		void SetColor(const Vector4& color) { m_Color = color; }
		void SetExposure(float exposure) { m_Exposure = exposure; }

		Vector4 GetColor() const { return m_Color; }
        float GetExposure() const { return m_Exposure; }

		LightType GetLightType() const { return m_LightType; }
		void SetLightType(LightType type) { m_LightType = type; }

		void SetCurrentCascade(int cascade) { m_CurrentCascade = cascade; }
		int GetCurrentCascade() const { return m_CurrentCascade; }

		const XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }
		const XMMATRIX& GetProjectionMatrix() const { return m_ProjectionMatrix; }

		static void SetActiveLight(Light* light) { s_ActiveLight = light; }
		static Light* GetActiveLight() { return s_ActiveLight; }
	};

}
