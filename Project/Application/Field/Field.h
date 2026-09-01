#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "UniverseMaterial.h"

class Field : public GameEngine::IGameObject
{
public:
    Field(GameEngine::Model* model);

    void Initialize() override;
    void Update() override;
    void Draw() override;

    // ゲッター
    float GetFieldRadius() const { return fieldRadius_; }

    void SetPlayerPos(Vector3 pos) {
        universeMaterial_.materialData_->PlayerPos = {pos.x,pos.z};
    }

private:
    GameEngine::ModelComponent modelComponent_;

    // パラメータ機能
    std::unique_ptr<GameEngine::DebugParameter> debugParame_;

    // 調整パラメータ
    float fieldRadius_ = 20.0f;
    float height_ = 1.0f; 

    GameEngine::UniverseMaterial universeMaterial_;
};