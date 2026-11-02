#include "ModelImporter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

using EngineCore::Render::Types::VERTEX;

// Wavefront OBJ の取り込み。旧 Engine/Source/Utility/OBJLoader.cpp の LoadModel / LoadMaterial の解析をそのまま移したもの。
//   - 左手系にするため、位置と法線の x を反転する
//   - テクスチャ座標の v を反転する（1 - v）
//   - 面ごとに先頭 2 つのインデックスを入れ替え、四角形は三角形 2 枚に分割する
// マテリアルからはベースカラーのテクスチャ名（map_Kd）だけを取る。
namespace AssetPipeline {

	namespace {
		struct ObjMaterial {
			std::string name;
			std::string textureBaseColor;
		};

		// .mtl から、マテリアル名とベースカラーのテクスチャ名を読む
		bool LoadMaterialTextures(const std::string& fileName, std::vector<ObjMaterial>& materials) {
			FILE* file = fopen(fileName.c_str(), "rt");
			if (!file) return false;

			char str[256];
			while (true) {
				if (fscanf(file, "%255s", str) != 1) break;

				if (strcmp(str, "newmtl") == 0) {
					ObjMaterial material;
					if (fscanf(file, "%255s", str) == 1) material.name = str;
					materials.push_back(std::move(material));
				}
				else if (strcmp(str, "map_Kd") == 0) {
					if (fscanf(file, "%255s", str) == 1 && !materials.empty())
						materials.back().textureBaseColor = str;
				}
			}

			fclose(file);
			return true;
		}
	}

	bool ImportObj(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh) {
		const size_t lastSlash = sourcePath.find_last_of("\\/");
		const std::string dir = lastSlash == std::string::npos ? std::string() : sourcePath.substr(0, lastSlash + 1);

		FILE* file = fopen(sourcePath.c_str(), "rt");
		if (!file) return false;

		unsigned int posNum = 0;
		unsigned int norNum = 0;
		unsigned int texNum = 0;
		unsigned int colNum = 0;

		unsigned int vertexNum = 0;
		unsigned int indexNum = 0;
		unsigned int in = 0;

		char str[256];
		char* s;
		int c;

		// 1 回目: 要素数を数える
		while (true) {
			if (fscanf(file, "%255s", str) != 1) break;

			if (strcmp(str, "v") == 0) {
				posNum++;
			}
			else if (strcmp(str, "vn") == 0) {
				norNum++;
			}
			else if (strcmp(str, "vc") == 0) {
				colNum++;
			}
			else if (strcmp(str, "vt") == 0) {
				texNum++;
			}
			else if (strcmp(str, "f") == 0) {
				in = 0;

				do {
					if (fscanf(file, "%255s", str) != 1) break;
					vertexNum++;
					in++;
					c = fgetc(file);
				} while (c != '\n' && c != '\r' && c != EOF);

				// 三角ポリゴンと四角ポリゴンのみ対応
				if (in == 4)
					in = 6;

				indexNum += in;
			}
		}

		std::vector<Vector3> posArray(posNum);
		std::vector<Vector3> norArray(norNum);
		std::vector<Vector2> texArray(texNum);
		std::vector<Vector4> colArray(colNum);

		std::vector<VERTEX> vertices(vertexNum);
		std::vector<unsigned int> indices(indexNum);

		std::vector<ObjMaterial> materials;
		std::vector<std::string> subsetTextures;

		Vector3* pos = posArray.data();
		Vector3* nor = norArray.data();
		Vector2* tex = texArray.data();
		Vector4* col = colArray.data();

		unsigned int vc = 0;
		unsigned int ic = 0;
		bool ok = true;

		// 2 回目: 値を読む
		fseek(file, 0, SEEK_SET);

		while (ok) {
			if (fscanf(file, "%255s", str) != 1) break;

			if (strcmp(str, "mtllib") == 0) {
				if (fscanf(file, "%255s", str) == 1) {
					if (!LoadMaterialTextures(dir + str, materials)) {
						fprintf(stderr, "Failed to open material: %s%s\n", dir.c_str(), str);
					}
				}
			}
			else if (strcmp(str, "v") == 0) {
				// position
				fscanf(file, "%f", &pos->x);
				fscanf(file, "%f", &pos->y);
				fscanf(file, "%f", &pos->z);
				pos->x *= -1.0f;
				pos++;
			}
			else if (strcmp(str, "vn") == 0) {
				// normal
				fscanf(file, "%f", &nor->x);
				fscanf(file, "%f", &nor->y);
				fscanf(file, "%f", &nor->z);
				nor->x *= -1.0f;
				nor++;
			}
			else if (strcmp(str, "vc") == 0) {
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
			}
			else if (strcmp(str, "usemtl") == 0) {
				// サブセットの開始。マテリアルのテクスチャ名をサブセット順に集める
				if (fscanf(file, "%255s", str) == 1) {
					for (const auto& material : materials) {
						if (material.name == str) {
							if (!material.textureBaseColor.empty())
								subsetTextures.push_back(material.textureBaseColor);
							break;
						}
					}
				}
			}
			else if (strcmp(str, "f") == 0) {
				// 面
				in = 0;

				do {
					if (fscanf(file, "%255s", str) != 1) break;

					s = strtok(str, "/");
					if (!s) { ok = false; break; }
					vertices[vc].Position = posArray[atoi(s) - 1];

					if (s[strlen(s) + 1] != '/') {
						// テクスチャ座標がある場合
						s = strtok(NULL, "/");
						if (s) vertices[vc].TexCoord = texArray[atoi(s) - 1];
					}

					s = strtok(NULL, "/");
					if (s) vertices[vc].Normal = norArray[atoi(s) - 1];

					s = strtok(NULL, "/");
					if (s)
						vertices[vc].Color = colArray[atoi(s) - 1];
					else
						vertices[vc].Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

					indices[ic] = vc;
					ic++;
					vc++;

					in++;
					c = fgetc(file);
				} while (c != '\n' && c != '\r' && c != EOF);

				if (!ok || in < 2) { ok = false; break; }

				std::swap(indices[ic - in], indices[ic - in + 1]);

				// 四角は三角に分割
				if (in == 4) {
					indices[ic] = vc - 2;
					ic++;
					indices[ic] = vc - 4;
					ic++;
				}
			}
		}

		fclose(file);

		if (!ok || vc != vertexNum || ic != indexNum) {
			fprintf(stderr, "Failed to parse OBJ: %s\n", sourcePath.c_str());
			return false;
		}

		mesh.vertices = std::move(vertices);
		mesh.indices = std::move(indices);
		mesh.topology = EngineCore::Render::PrimitiveTopology::TriangleList;
		mesh.textures = std::move(subsetTextures);
		return true;
	}
}
