#pragma once

#include "RenderManager.h"
#include "VertexData.h"
#include "Code/Render/Material.h"

// 描画サブセットマテリアル構造体
struct SUBSET_MATERIAL {
  char Name[256];
  Render::Material Material;

  std::unique_ptr<Render::Types::TEXTURE> TextureBaseColor;
};

// 描画サブセット構造体
struct SUBSET {
  char Name[256];
  unsigned int StartIndex;
  unsigned int IndexNum;
  SUBSET_MATERIAL Material;
};

// モデルサブセットマテリアル構造体
struct MODEL_SUBSET_MATERIAL {
  char Name[256];
  Render::Types::MATERIAL Material;

  char TextureNameBaseColor[256];
};

// モデルサブセット構造体
struct MODEL_SUBSET {
  char Name[256];
  unsigned int StartIndex;
  unsigned int IndexNum;
  MODEL_SUBSET_MATERIAL Material;
};

// モデル構造体
struct MODEL {
  Render::Types::VERTEX *VertexArray;
  unsigned int VertexNum;

  unsigned int *IndexArray;
  unsigned int IndexNum;

  MODEL_SUBSET *SubsetArray;
  unsigned int SubsetNum;
};

class VertexData;

void LoadObj(const char *, VertexData*);

void LoadObjToVertexData(const char *FileName, VertexData *vertexData);