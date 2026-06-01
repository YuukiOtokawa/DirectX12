#include "MeshFilter.h"
#include "../../../ImGui/Code/imgui.h"

#include "../../Utility/FilePicker.h"
#include "../../Utility/FBXLoader.h"
#include "../../Utility/OBJLoader.h"
#include <algorithm>

#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"

void EngineCore::General::MeshFilter::Update() {}

void EngineCore::General::MeshFilter::Inspector() {
	if (ImGui::Button("Open Model")) {
		auto filePath = OpenFileDialog();
		if (filePath.empty())
			return;

		if (!_pVertexData) {
			_pVertexData = new VertexData();
		}

		// Determine file extension
		std::string pathStr(filePath);
		std::string ext = pathStr.substr(pathStr.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == "fbx") {
			auto fbxData = LoadFBX(filePath.c_str(), _pVertexData);
			if (fbxData._Scene) {
				SetVertexData(fbxData.vertexData[0]);
				SetPrimitiveTopology(fbxData.vertexData[0]->GetPrimitiveTopology());
			}
		}
		else if (ext == "obj") {
			LoadObjToVertexData(filePath.c_str(), _pVertexData);
			SetVertexData(_pVertexData->GetFilePath().c_str(), _pVertexData->GetVertices(), _pVertexData->GetIndices());
			SetPrimitiveTopology(_pVertexData->GetPrimitiveTopology());
		}
	}
}

void EngineCore::General::MeshFilter::SetVertexData(const char* FilePath, std::vector<Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	HRESULT hr;
	{
		auto render = Render::RenderManager::GetInstance();

		_VertexBuffer = render->CreateVertexBuffer(sizeof(Render::Types::VERTEX), (unsigned int)vertices.size());
		Render::Types::VERTEX* buffer{};
		hr = _VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);

		memcpy(buffer, vertices.data(), sizeof(Render::Types::VERTEX) * vertices.size());
		_VertexBuffer->Resource->Unmap(0, nullptr);
	}

	if (!indices.empty()) {
		auto render = Render::RenderManager::GetInstance();
		_IndexBuffer = render->CreateIndexBuffer((unsigned int)indices.size());
		unsigned int* buffer{};
		hr = _IndexBuffer->Resource->Map(0, nullptr, (void**)&buffer);
		memcpy(buffer, indices.data(), sizeof(unsigned int) * indices.size());
		_IndexBuffer->Resource->Unmap(0, nullptr);
	}
}
