#pragma once

#include "../assimp/cimport.h"
#include "../assimp/scene.h"
#include "../assimp/postprocess.h"
#include "../assimp/matrix4x4.h"

#include "../Utility/VertexData.h"

#include "../Render/RenderManager.h"

using namespace Render::Types;

#define NOMINMAX

struct DEFORM_VERTEX {
	aiVector3D position;
	aiVector3D normal;
	int boneNum;
	std::string boneName[5];
	float boneWeight[5];
};


struct FBXData {
	const aiScene* _Scene = nullptr;

	std::vector<std::unique_ptr<TEXTURE>> textures;

	std::vector<MATERIAL> materials;
	std::vector<UINT> baseColorTexOfMaterial;

	VertexData** vertexData = nullptr; // [mesh][vertex]
	std::vector<DEFORM_VERTEX>* deformVertex = nullptr; // [mesh][vertex]
};

FBXData LoadFBX(const char* FileName, VertexData* pVertexData);