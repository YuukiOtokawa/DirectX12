
#include "OBJLoader.h"
#include "VertexData.h"
#include "Utility/VectorClass.h"
#include <shlwapi.h>
#include <stdio.h>

#pragma comment(lib, "shlwapi.lib")

using namespace EngineCore::Render;

void LoadObjBin(const char *FileName, MODEL *Model) {
    FILE *file;
    file = fopen(FileName, "rb");
    assert(file);

    fread(&Model->VertexNum, sizeof(Model->VertexNum), 1, file);
    Model->VertexArray = new VERTEX[Model->VertexNum];

    fread(Model->VertexArray, sizeof(VERTEX), Model->VertexNum, file);

    fread(&Model->IndexNum, sizeof(Model->IndexNum), 1, file);
    Model->IndexArray = new unsigned int[Model->IndexNum];

    fread(Model->IndexArray, sizeof(unsigned int), Model->IndexNum, file);

    fread(&Model->SubsetNum, sizeof(Model->SubsetNum), 1, file);
    Model->SubsetArray = new MODEL_SUBSET[Model->SubsetNum];

    fread(Model->SubsetArray, sizeof(MODEL_SUBSET), Model->SubsetNum, file);

    fclose(file);
}

void SaveObjBin(const char *FileName, MODEL *Model) {
    FILE *file;
    file = fopen(FileName, "wb");
    assert(file);

    fwrite(&Model->VertexNum, sizeof(Model->VertexNum), 1, file);
    fwrite(Model->VertexArray, sizeof(VERTEX), Model->VertexNum, file);

    fwrite(&Model->IndexNum, sizeof(Model->IndexNum), 1, file);
    fwrite(Model->IndexArray, sizeof(unsigned int), Model->IndexNum, file);

    fwrite(&Model->SubsetNum, sizeof(Model->SubsetNum), 1, file);
    fwrite(Model->SubsetArray, sizeof(MODEL_SUBSET), Model->SubsetNum, file);

    fclose(file);
}

void LoadMaterial(const char *FileName, MODEL_SUBSET_MATERIAL *&MaterialArray,
                  UINT *MaterialNum) {
  char str[256];

  FILE *file = fopen(FileName, "rt");
  assert(file);

  MODEL_SUBSET_MATERIAL *materialArray = nullptr;
  unsigned int materialNum = 0;

  while (true) {
    fscanf(file, "%s", str);

    if (feof(file))
      break;

    if (strcmp(str, "newmtl") == 0) {
      materialNum++;
    }
  }

  materialArray = new MODEL_SUBSET_MATERIAL[materialNum];

  int mc = -1;

  fseek(file, 0, SEEK_SET);

  while (true) {
    fscanf(file, "%s", str);

    if (feof(file) != 0)
      break;

    if (strcmp(str, "newmtl") == 0) {
      mc++;
      fscanf(file, "%s", materialArray[mc].Name);

      strcpy(materialArray[mc].TextureNameBaseColor, "");
    } else if (strcmp(str, "Ka") == 0) {
      float ambient;
      fscanf(file, "%f", &ambient);
      fscanf(file, "%f", &ambient);
      fscanf(file, "%f", &ambient);

    } else if (strcmp(str, "Kd") == 0) {
      fscanf(file, "%f", &materialArray[mc].Material.BaseColor.x);
      fscanf(file, "%f", &materialArray[mc].Material.BaseColor.y);
      fscanf(file, "%f", &materialArray[mc].Material.BaseColor.z);
      materialArray[mc].Material.BaseColor.w = 1.0f;
    } else if (strcmp(str, "Ks") == 0) {
      float specular;
      fscanf(file, "%f", &specular);
      fscanf(file, "%f", &specular);
      fscanf(file, "%f", &specular);

      materialArray[mc].Material.Specular = specular;
    } else if (strcmp(str, "Ns") == 0) {
      float shininess;
      fscanf(file, "%f", &shininess);
    } else if (strcmp(str, "d") == 0) {
      fscanf(file, "%f", &materialArray[mc].Material.BaseColor.w);
    } else if (strcmp(str, "Metallic") == 0) {
      fscanf(file, "%f", &materialArray[mc].Material.Metallic);
    } else if (strcmp(str, "Roughness") == 0) {
      fscanf(file, "%f", &materialArray[mc].Material.Roughness);
    } else if (strcmp(str, "map_Kd") == 0) {
      fscanf(file, "%s", str);
      strcat(materialArray[mc].TextureNameBaseColor, str);
    }
  }

  fclose(file);

  MaterialArray = materialArray;
  *MaterialNum = materialNum;
}


void LoadModel(const char *FileName, MODEL *Model) {
    std::string dir(FileName);
    dir = dir.substr(0, dir.find_last_of("\\"));

    Vector3 *posArray = nullptr;
    Vector3 *norArray = nullptr;
    Vector2 *texArray = nullptr;
    Vector4 *colArray = nullptr;

    unsigned int posNum = 0;
    unsigned int norNum = 0;
    unsigned int texNum = 0;
    unsigned int colNum = 0;

    unsigned int vertexNum = 0;
    unsigned int indexNum = 0;
    unsigned int in = 0;
    unsigned int subsetNum = 0;

    MODEL_SUBSET_MATERIAL *matArray = nullptr;
    unsigned int matNum = 0;

    char str[256];
    char *s;
    char c;

    FILE *file = fopen(FileName, "rt");
    assert(file);

    // read file
    while (true) {
        fscanf(file, "%s", str);

        if (feof(file))
            break;

        if (strcmp(str, "v") == 0) {
            posNum++;
        } else if (strcmp(str, "vn") == 0) {
            norNum++;
        } else if (strcmp(str, "vc") == 0) {
            colNum++;
        } else if (strcmp(str, "vt") == 0) {
            texNum++;
        } else if (strcmp(str, "usemtl") == 0) {
            subsetNum++;
        } else if (strcmp(str, "f") == 0) {
            in = 0;

            do {
                fscanf(file, "%s", str);
                vertexNum++;
                in++;
                c = fgetc(file);
            } while (c != '\n' && c != '\r');

            // 三角ポリゴンのみ対応
            if (in == 4)
              in = 6;

            indexNum += in;
        }
    }

    posArray = new Vector3[posNum];
    norArray = new Vector3[norNum];
    texArray = new Vector2[texNum];
    colArray = new Vector4[colNum];


    
	Model->VertexArray = new VERTEX[ vertexNum ];
	Model->VertexNum = vertexNum;

	Model->IndexArray = new unsigned int[ indexNum ];
	Model->IndexNum = indexNum;

	Model->SubsetArray = new MODEL_SUBSET[ subsetNum ];
	Model->SubsetNum = subsetNum;




	//頂点データ読み込み
	Vector3* pos = posArray;
	Vector3* nor = norArray;
	Vector2* tex = texArray;
	Vector4* col = colArray;

    UINT vc = 0;
    UINT ic = 0;
    UINT sc = 0;

    char objName[256];

    fseek(file, 0, SEEK_SET);

    while (true) {
        fscanf(file, "%s", str);

        if (feof(file) != 0)
            break;

        if (strcmp(str, "mtllib") == 0) {
            // mtllib
            fscanf(file, "%s", str);

            char path[256];
            strcpy(path, dir.c_str());
            strcat(path, "\\");
            strcat(path, str);

        LoadMaterial(path, matArray, &matNum);
      } else if (strcmp(str, "o") == 0) {
        // objectName
        fscanf(file, "%s", objName);
      } else if (strcmp(str, "v") == 0) {
        // position
        fscanf(file, "%f", &pos->x);
        fscanf(file, "%f", &pos->y);
        fscanf(file, "%f", &pos->z);
        pos->x *= -1.0f;
        //*pos *= 100.0f;
        pos++;
      } else if (strcmp(str, "vn") == 0) {
        // normal
        fscanf(file, "%f", &nor->x);
        fscanf(file, "%f", &nor->y);
        fscanf(file, "%f", &nor->z);
        nor->x *= -1.0f;
        nor++;
      } else if (strcmp(str, "vc") == 0) {
        // color
        fscanf(file, "%f", &col->x);
        fscanf(file, "%f", &col->y);
        fscanf(file, "%f", &col->z);
        fscanf(file, "%f", &col->w);
        col++;
      }

      else if (strcmp(str, "vt") == 0) {
        // texcoord
        fscanf(file, "%f", &tex->x);
        fscanf(file, "%f", &tex->y);
        tex->y = 1.0f - tex->y;
        tex++;
      } else if (strcmp(str, "usemtl") == 0) {
        // mtllib
        fscanf(file, "%s", str);

        strcpy(Model->SubsetArray[sc].Name, objName);

        if (sc != 0)
          Model->SubsetArray[sc - 1].IndexNum =
              ic - Model->SubsetArray[sc - 1].StartIndex;

        Model->SubsetArray[sc].StartIndex = ic;

        for (unsigned int i = 0; i < matNum; i++) {
          if (strcmp(str, matArray[i].Name) == 0) {
            Model->SubsetArray[sc].Material = matArray[i];
            break;
          }
        }

        sc++;

      } else if (strcmp(str, "f") == 0) {
        // 面
        in = 0;

        do {
          fscanf(file, "%s", str);

          s = strtok(str, "/");
          Model->VertexArray[vc].Position = posArray[atoi(s) - 1];

          if (s[strlen(s) + 1] != '/') {
            // テクスチャ座標がある場合
            s = strtok(NULL, "/");
            Model->VertexArray[vc].TexCoord = texArray[atoi(s) - 1];
          }

          s = strtok(NULL, "/");
          Model->VertexArray[vc].Normal = norArray[atoi(s) - 1];

          s = strtok(NULL, "/");
          if (s)
            Model->VertexArray[vc].Color = colArray[atoi(s) - 1];
          else
            Model->VertexArray[vc].Color = {1.0f, 1.0f, 1.0f, 1.0f};

          Model->IndexArray[ic] = vc;
          ic++;
          vc++;

          in++;
          c = fgetc(file);
        } while (c != '\n' && c != '\r');

        std::swap(Model->IndexArray[ic - in], Model->IndexArray[ic - in + 1]);

        // 四角は三角に分割
        if (in == 4) {
          Model->IndexArray[ic] = vc - 2;
          ic++;
          Model->IndexArray[ic] = vc - 4;
          ic++;
        }
      }
    }

  if (sc != 0)
    Model->SubsetArray[sc - 1].IndexNum =
        ic - Model->SubsetArray[sc - 1].StartIndex;

  fclose(file);

  assert(posArray);
  assert(norArray);
  assert(texArray);
  assert(colArray);

  delete[] posArray;
  delete[] norArray;
  delete[] texArray;
  delete[] colArray;

  if (matArray) {
    delete[] matArray;
  }
}

void LoadObj(const char *FileName) {
  auto render = RenderManager::GetInstance();

  std::string dir(FileName);
  dir = dir.substr(0, dir.find_last_of("\\"));
  dir += "\\";

  std::string path(FileName);
  path = path.substr(0, path.find_last_of("."));
  path += ".objbin";

  MODEL model{};

  {
    bool findBin = false;

    HANDLE hBin = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBin != INVALID_HANDLE_VALUE) {
      HANDLE hObj = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
      if (hObj == INVALID_HANDLE_VALUE)
        assert(false);

      FILETIME tBin, tObj;
      GetFileTime(hBin, NULL, NULL, &tBin);
      GetFileTime(hObj, NULL, NULL, &tObj);

      if (CompareFileTime(&tBin, &tObj) >= 0)
        findBin = true;

      CloseHandle(hObj);
    }
    CloseHandle(hBin);

    if (findBin) {
      LoadObjBin(path.c_str(), &model);
    } else {
      LoadModel(FileName, &model);
      SaveObjBin(path.c_str(), &model);
    }
  }

  {
    auto device = RenderManager::GetInstance()->GetDevice();

    auto vertexBuffer =
        render->CreateVertexBuffer(sizeof(VERTEX), model.VertexNum);
    auto indexBuffer = render->CreateIndexBuffer(model.IndexNum);

    VERTEX *vertex;
    vertexBuffer->Resource->Map(0, nullptr, (void **)&vertex);
    memcpy(vertex, model.VertexArray, sizeof(VERTEX) * model.VertexNum);
    vertexBuffer->Resource->Unmap(0, nullptr);

    unsigned int *index;
    indexBuffer->Resource->Map(0, nullptr, (void **)&index);
    memcpy(index, model.IndexArray, sizeof(unsigned int) * model.IndexNum);
    indexBuffer->Resource->Unmap(0, nullptr);
  }

  std::vector<SUBSET> subsetArray;
  {
    subsetArray.resize(model.SubsetNum);

    for (unsigned int i = 0; i < model.SubsetNum; i++) {
      strcpy(subsetArray[i].Name, model.SubsetArray[i].Name);

      subsetArray[i].StartIndex = model.SubsetArray[i].StartIndex;
      subsetArray[i].IndexNum = model.SubsetArray[i].IndexNum;
      subsetArray[i].Material.Material.SetFromConstant(model.SubsetArray[i].Material.Material);

      strcpy(subsetArray[i].Material.Name, model.SubsetArray[i].Material.Name);

      if (strlen(model.SubsetArray[i].Material.TextureNameBaseColor) != 0)
        subsetArray[i].Material.TextureBaseColor =
            RenderManager::GetInstance()->LoadTexture(
                (dir + model.SubsetArray[i].Material.TextureNameBaseColor)
                    .c_str());
    }
  }

  std::sort(subsetArray.begin(), subsetArray.end(),
            [](const SUBSET &a, const SUBSET &b) {
              return a.Material.Material.GetBaseColor().w >
                     b.Material.Material.GetBaseColor().w;
            });

  assert(model.VertexArray);
  assert(model.IndexArray);
  assert(model.SubsetArray);

  delete[] model.VertexArray;
  delete[] model.IndexArray;
  delete[] model.SubsetArray;
}

void LoadObjToVertexData(const char* FileName, VertexData* pOutVertexData) {
  if (!pOutVertexData) return;

  std::string dir(FileName);
  dir = dir.substr(0, dir.find_last_of("\\"));
  dir += "\\";

  std::string path(FileName);
  path = path.substr(0, path.find_last_of("."));
  path += ".objbin";

  MODEL model{};

  // Load from binary cache or parse OBJ
  {
    bool findBin = false;
    HANDLE hBin = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBin != INVALID_HANDLE_VALUE) {
      HANDLE hObj = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
      if (hObj == INVALID_HANDLE_VALUE)
        assert(false);

      FILETIME tBin, tObj;
      GetFileTime(hBin, NULL, NULL, &tBin);
      GetFileTime(hObj, NULL, NULL, &tObj);

      if (CompareFileTime(&tBin, &tObj) >= 0)
        findBin = true;

      CloseHandle(hObj);
    }
    CloseHandle(hBin);

    if (findBin) {
      LoadObjBin(path.c_str(), &model);
    } else {
      LoadModel(FileName, &model);
      SaveObjBin(path.c_str(), &model);
    }
  }

  // Transfer vertices and indices to VertexData
  std::vector<EngineCore::Render::Types::VERTEX> vertices(model.VertexArray, model.VertexArray + model.VertexNum);
  std::vector<unsigned int> indices(model.IndexArray, model.IndexArray + model.IndexNum);

  pOutVertexData->SetVertices(vertices);
  pOutVertexData->SetIndices(indices);
  pOutVertexData->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pOutVertexData->SetFilePath(FileName);

  // Load textures
  std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>> textures;
  for (unsigned int i = 0; i < model.SubsetNum; i++) {
    if (strlen(model.SubsetArray[i].Material.TextureNameBaseColor) != 0) {
      auto tex = RenderManager::GetInstance()->LoadTexture(
          (dir + model.SubsetArray[i].Material.TextureNameBaseColor).c_str());
      if (tex) {
        textures.push_back(std::move(tex));
      }
    }
  }
  pOutVertexData->SetTextures(std::move(textures));

  // Memory cleanup
  delete[] model.VertexArray;
  delete[] model.IndexArray;
  delete[] model.SubsetArray;
}