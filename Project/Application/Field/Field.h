#pragma once
#include <vector>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "UniverseMaterial.h"

class Field : public GameEngine::IGameObject
{
public:
    Field(GameEngine::Model* model, GameEngine::Model* poleModel, GameEngine::Model* circleModel, GameEngine::Model* planeModel);

    void Initialize() override;
    void Update() override;
    void Draw() override;

    // ゲッター
    float GetFieldRadius() const { return fieldRadius_; }

private:
    GameEngine::ModelComponent modelComponent_;

    // パラメータ機能
    std::unique_ptr<GameEngine::DebugParameter> debugParame_;

    // 調整パラメータ
    float fieldRadius_ = 20.0f;
    float height_ = 1.0f; 

    // ステージの外周
    std::vector<std::unique_ptr<GameEngine::ModelComponent>> poleModels_;

    // ステージの円
    std::vector<std::unique_ptr<GameEngine::ModelComponent>> circleModels_;

    // 宇宙
    GameEngine::ModelComponent universeModel_;
    GameEngine::UniverseMaterial universeMaterial_;

    // 円の色アニメーション
    float colorTime_ = 0.0f;   // 経過時間
    float colorCycle_ = 4.0f;   // 1周にかかる秒数
    float colorOffset_ = 0.125f; // リング1枚ごとの位相ずれ
};