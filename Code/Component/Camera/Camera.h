#pragma once

#include <DirectXMath.h>
using namespace DirectX;

class Camera {
	XMFLOAT3 m_Position;
	XMFLOAT3 m_Target;

public:
	Camera();
	~Camera() = default;

	void SetPosition(const XMFLOAT3& Position) { 
		if (Position.x == m_Target.x && Position.y == m_Target.y && Position.z == m_Target.z)
			return;
		m_Position = Position;
	}
	void SetTarget(const XMFLOAT3& Target) { 
		if (Target.x == m_Position.x && Target.y == m_Position.y && Target.z == m_Position.z)
			return;
		m_Target = Target;
	}

	XMFLOAT3 GetPosition() const { return m_Position; }
	XMFLOAT3 GetTarget() const { return m_Target; }

	void Update();
	void Draw();

};