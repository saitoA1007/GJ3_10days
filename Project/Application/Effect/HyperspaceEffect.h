#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "HyperspaceMaterial.h"

class HyperspaceEffect : public GameEngine::IGameObject {
public:
    HyperspaceEffect(GameEngine::Model* model);

    void Initialize() override;
    void Update() override;
    void Draw() override;

public:

    void StartAnimation() {
        if (hyperspacePhase_ == GameEngine::HyperspacePhase::Idle)
        {
            hyperspacePhase_ = GameEngine::HyperspacePhase::Jump;
        } else
        {
            hyperspacePhase_ = GameEngine::HyperspacePhase::Idle;
        }

        hyperspacePhaseTime_ = 0.0f;
        hyperspaceMaterial_.materialData_->phase = static_cast<uint32_t>(hyperspacePhase_);
        hyperspaceMaterial_.materialData_->phaseTime = 0.0f;
    }

    void ResetAnimation() {
        hyperspacePhase_ = GameEngine::HyperspacePhase::Idle;
        hyperspacePhaseTime_ = 0.0f;
        hyperspaceMaterial_.materialData_->phase = static_cast<uint32_t>(hyperspacePhase_);
        hyperspaceMaterial_.materialData_->phaseTime = 0.0f;
    }

private:
    // パラメータ機能
    std::unique_ptr<GameEngine::DebugParameter> debugParame_;
    GameEngine::ModelComponent halfDomeModel_;
    GameEngine::HyperspaceMaterial hyperspaceMaterial_;
    GameEngine::HyperspacePhase hyperspacePhase_ = GameEngine::HyperspacePhase::Idle;
    float hyperspacePhaseTime_ = 0.0f;
    float kHyperspaceJumpDuration_ = 2.0f;
};