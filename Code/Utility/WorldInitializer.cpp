#include "WorldInitializer.h"

#include "../Manager/ObjectManager.h"
#include "../GameObject/GameObject.h"
#include "../Component/Transform/Transform.h"
#include "../Component/Polygon/MeshFilter.h"
#include "../Component/Polygon/MeshRenderer.h"
#include "../Component/Polygon/Light.h"
#include "../Component/Camera/Camera.h"
#include "../Render/RenderManager.h"

#include "FilePicker.h"
#include "FBXLoader.h"
#include "OBJLoader.h"


namespace EngineCore::Utility {

    void InitializeWorld(EngineCore::Manager::ObjectManager* objectManager) {
        if (!objectManager) return;

        // 1. 水平な正方形の上向き板ポリゴン
        // Pos(0.0,0.0,0.0) Scale(5,5,5) Rot(0,0,0)
        auto planeObj = objectManager->CreateObject();
        planeObj->SetName("GroundPlane");

        // Transform追加
        planeObj->AddComponent<General::Transform>();
        auto planeTransform = planeObj->GetComponent<General::Transform>();
        if (planeTransform) {
            planeTransform->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
            planeTransform->SetScale(Vector3(5.0f, 5.0f, 5.0f));
            planeTransform->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        }

        // 頂点データとインデックスデータの用意 (上向き板ポリゴン)
        // 頂点定義: 左上, 右上, 左下, 右下
        std::vector<EngineCore::Render::Types::VERTEX> vertices = {
            { Vector3(-0.5f, 0.0f,  0.5f), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 0.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) }, // 左上
            { Vector3( 0.5f, 0.0f,  0.5f), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 0.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) }, // 右上
            { Vector3(-0.5f, 0.0f, -0.5f), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) }, // 左下
            { Vector3( 0.5f, 0.0f, -0.5f), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) }  // 右下
        };
        std::vector<unsigned int> indices = {
            0, 1, 2, // 1つ目の三角形 (左上 -> 右上 -> 左下)
            2, 1, 3  // 2つ目の三角形 (左下 -> 右上 -> 右下)
        };

        // VertexDataオブジェクトの作成
        auto vertexData = new VertexData();
        vertexData->SetFilePath("Plane");
        vertexData->SetVertices(vertices);
        vertexData->SetIndices(indices);
        vertexData->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // テクスチャのロードと設定
        auto renderManager = EngineCore::Render::RenderManager::GetInstance();
        if (renderManager) {
            auto texture = renderManager->LoadTexture("Assets/field004.dds");
            if (texture) {
                std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>> textures;
                textures.push_back(std::move(texture));
                vertexData->SetTextures(std::move(textures));
            }
        }

        // MeshFilter と MeshRenderer の追加
        planeObj->AddComponent<General::MeshFilter>();
        auto meshFilter = planeObj->GetComponent<General::MeshFilter>();
        if (meshFilter) {
            meshFilter->SetVertexData(vertexData);
        }
        auto meshRenderer = planeObj->AddComponent<General::MeshRenderer>();
        meshRenderer->SetShader("Geometry"); // デフォルトのジオメトリシェーダーを使用)
        meshRenderer->GetMaterial().SetRenderPassType(EngineCore::Render::RenderPassType::DeferredOpaque);
        meshRenderer->GetMaterial().SetShaderFilePath("Code/Shader/Geometry.hlsl");

        {
            // 2. 天球
            // Pos(0.0,0.0,0.0) Scale(5,5,5) Rot(0,0,0)
            auto skyObj = objectManager->CreateObject();
            skyObj->SetName("SkyDome");

            // Transform追加
            skyObj->AddComponent<General::Transform>();
            auto skyTransform = skyObj->GetComponent<General::Transform>();
            if (skyTransform) {
                skyTransform->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
                skyTransform->SetScale(Vector3(10.0f, 10.0f, 10.0f));
                skyTransform->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
            }

            auto skyVertexData = new VertexData();
            LoadObjToVertexData("Assets\\sky.obj", skyVertexData);


            // MeshFilter と MeshRenderer の追加
            skyObj->AddComponent<General::MeshFilter>();
            auto meshRenderer = skyObj->AddComponent<General::MeshRenderer>();
            auto meshFilter = skyObj->GetComponent<General::MeshFilter>();
            if (meshFilter) {
                meshFilter->SetVertexData(skyVertexData);
                meshFilter->SetPrimitiveTopology(skyVertexData->GetPrimitiveTopology());
            }
            meshRenderer->SetShader("Unlit"); // スカイドーム用のシェーダーを使用
            meshRenderer->GetMaterial().SetRenderPassType(EngineCore::Render::RenderPassType::ForwardOpaque);
            meshRenderer->GetMaterial().SetShaderFilePath("Code/Shader/Unlit.hlsl");
        }

        // 3. ライトオブジェクト
        auto lightObj = objectManager->CreateObject();
        lightObj->SetName("DirectionalLight");
        
        lightObj->AddComponent<General::Transform>();
        auto lightTransform = lightObj->GetComponent<General::Transform>();
        if (lightTransform) {
            lightTransform->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
            // 少し斜め下（地面方向）を照らすように回転を設定 (Pitch: 0.7f, Yaw: 0.7f)
            lightTransform->SetRotation(Vector3(-0.5f, 0.5f, 0.0f));
        }
        lightObj->AddComponent<General::Light>();

        // 4. GameView用カメラ
        auto cameraObj = objectManager->CreateObject();
        cameraObj->SetName("GameCamera"); // "EditorCamera" 以外の名前で登録

        cameraObj->AddComponent<General::Transform>();
        auto cameraTransform = cameraObj->GetComponent<General::Transform>();
        if (cameraTransform) {
            cameraTransform->SetPosition(Vector3(0.0f, 4.0f, -10.0f));
            cameraTransform->SetRotation(Vector3(0.3f, 0.0f, 0.0f)); // 少し下を向く
        }
        cameraObj->AddComponent<General::Camera>();
    }
}
