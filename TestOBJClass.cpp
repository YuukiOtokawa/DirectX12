#include "TestOBJClass.h"
#include "Code/Utility/ResourcePath.h"

TestOBJClass::TestOBJClass() {
	model.Load(ASSET_DIR "torus.obj");
}

void TestOBJClass::Update() {
	rotation.y += 0.01f;
}

void TestOBJClass::Draw() {
	auto render = EngineCore::Render::RenderManager::GetInstance();

	{
		EngineCore::Render::Types::ENV_CONSTANT constant;

		constant.LightDirection = { 0.0f, 1.0f, 0.0f, 0.0f };
		constant.LightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		render->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::ENV, &constant, sizeof(constant));
	}

	{
		//マトリクス設定
		XMMATRIX world = XMMatrixIdentity();
		world *= XMMatrixScaling(scale.x, scale.y, scale.z);
		world *= XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		world *= XMMatrixTranslation(position.x, position.y, position.z);
		EngineCore::Render::Types::OBJECT_CONSTANT objectConstant{};
		XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));
		render->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	{
		XMMATRIX view = XMMatrixLookAtLH(
			XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f), // camera
			XMVectorSet(position.x,position.y,position.z, 1.0f), // target
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)  // up
		);

		float aspect = static_cast<float>(render->GetBackBufferWidth()) /
			static_cast<float>(render->GetBackBufferHeight());

		XMMATRIX projection = XMMatrixPerspectiveFovLH(
			XMConvertToRadians(60.0f),
			aspect,
			0.1f,
			1000.0f
		);

		EngineCore::Render::Types::CAMERA_CONSTANT cameraConstant{};
		XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(projection));

		render->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
	}

	model.Draw();
}
