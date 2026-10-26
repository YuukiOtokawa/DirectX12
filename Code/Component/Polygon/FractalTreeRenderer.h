#pragma once

#include "MeshRenderer.h"
#include <algorithm>

namespace EngineCore::General {

	// The source mesh's branch axis is local +Y, centered on X=Z=0.
	class FractalTreeRenderer : public MeshRenderer {
		REGISTER_COMPONENT(FractalTreeRenderer)

	public:
		void Draw() override;
		void Inspector() override;

		void SetBranchDepth(int depth) {
			m_BranchDepth = std::clamp(depth, 0, MaxBranchDepth);
		}
		void SetBranchAngle(float degrees) {
			m_BranchAngleDegrees = std::clamp(degrees, 0.0f, 90.0f);
		}

	private:
		// Keep recursive draw cost bounded; branch count grows exponentially.
		static constexpr int MaxBranchDepth = 5;
		int m_BranchDepth = 3;
		float m_BranchAngleDegrees = 30.0f;
		float m_BranchLength = 1.0f;
		float m_ChildScale = 0.7f;

		void DrawBranch(const DirectX::XMMATRIX& world,
			const DirectX::XMMATRIX& meshToBranch, int remainingDepth);
	};
}
