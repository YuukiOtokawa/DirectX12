#pragma once

namespace EngineCore::Manager {
    class ObjectManager;
}

namespace EngineCore::Utility {
    // 起動時にプリセットオブジェクト（板ポリゴン、ライト、GameView用カメラ）をワールドに追加する初期化処理
    void InitializeWorld(EngineCore::Manager::ObjectManager* objectManager);
}
