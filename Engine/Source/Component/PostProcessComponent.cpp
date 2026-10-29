#include "PostProcessComponent.h"
#include "RenderManager.h"
#include "Engine/Source/Utility/ResourcePath.h"

using namespace EngineCore::General;

PostProcessComponent::PostProcessComponent() {
	// デフォルトで InvertColor を登録しておく
	m_Passes.push_back({ "InvertColor", EngineCore::ResourcePath::Shader("InvertColor.hlsl") });
}

void PostProcessComponent::Start() {
	// シェーダ登録済みのこのタイミングで各パスのマテリアルを構成
	for (auto& pass : m_Passes) {
		SetupPassMaterial(pass);
	}
	ApplyPassesToRenderManager();
}

void PostProcessComponent::Update() {
}

void PostProcessComponent::SetupPassMaterial(PostProcessPassInfo& pass) {
	// メタデータを引き、バッファサイズの確定とデフォルト値の初期化を行う
	pass.material.SetShaderFile(pass.shaderPath);
}

void PostProcessComponent::ApplyPassesToRenderManager() {
	auto* renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	renderManager->ClearPostProcessPasses();
	for (const auto& pass : m_Passes) {
		// パス独自のプロパティバッファも一緒に渡す（b3にバインドされる）
		renderManager->AddPostProcessPass(pass.name, pass.shaderPath, pass.material.GetBufferData(), pass.material.GetBufferSize());
	}
}

bool PostProcessComponent::AddPass(const std::string& name, const std::string& shaderPath) {
	auto* renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return false;

	// コンパイルが通ったときだけ追加する
	if (!renderManager->ReloadPipeline(shaderPath, EngineCore::Render::RenderPassType::PostProcess)) return false;

	m_Passes.push_back({ name, shaderPath });
	// 登録直後にメタデータが利用可能なのでマテリアルを構成
	SetupPassMaterial(m_Passes.back());
	ApplyPassesToRenderManager();
	return true;
}

void PostProcessComponent::RemovePass(size_t index) {
	if (index >= m_Passes.size()) return;
	m_Passes.erase(m_Passes.begin() + index);
	ApplyPassesToRenderManager();
}
