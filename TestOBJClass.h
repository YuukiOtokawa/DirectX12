#pragma once

#include "Model.h"

#include <DirectXMath.h>
using namespace DirectX;

class TestOBJClass {
	XMFLOAT3 position{ 0.0f, -2.0f, 1.0f };
	XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

	Model model;

public:
	TestOBJClass();

	void Update();
	void Draw();
};

