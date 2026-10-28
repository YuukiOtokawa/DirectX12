#pragma once

#include "MeshRenderer.h"
#include <algorithm>

namespace EngineCore::General {

	// The source mesh's branch axis is local +Y, centered on X=Z=0.
	class FractalTreeRenderer : public MeshRenderer {
		REGISTER_COMPONENT(FractalTreeRenderer)

	public:
		// Keep recursive draw cost bounded; branch count grows exponentially.
		static constexpr int MaxBranchDepth = 5;

		void Draw() override;

		void SetBranchDepth(int depth) {
			m_BranchDepth = std::clamp(depth, 0, MaxBranchDepth);
		}
		void SetBranchAngle(float degrees) {
			m_BranchAngleDegrees = std::clamp(degrees, 0.0f, 90.0f);
		}
		void SetBranchLength(float length) {
			m_BranchLength = std::clamp(length, 0.001f, 1000.0f);
		}
		void SetChildScale(float scale) {
			m_ChildScale = std::clamp(scale, 0.1f, 0.95f);
		}

		int GetBranchDepth() const { return m_BranchDepth; }
		float GetBranchAngle() const { return m_BranchAngleDegrees; }
		float GetBranchLength() const { return m_BranchLength; }
		float GetChildScale() const { return m_ChildScale; }

	private:
		int m_BranchDepth = 3;
		float m_BranchAngleDegrees = 30.0f;
		float m_BranchLength = 1.0f;
		float m_ChildScale = 0.7f;

		void DrawBranch(const DirectX::XMMATRIX& world,
			const DirectX::XMMATRIX& meshToBranch, int remainingDepth);
	};
}
