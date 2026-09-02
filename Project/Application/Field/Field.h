#pragma once
#include <vector>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"

class Field : public GameEngine::IGameObject
{
public:
    Field(GameEngine::Model* model, GameEngine::Model* poleModel, GameEngine::Model* circleModel);

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
};