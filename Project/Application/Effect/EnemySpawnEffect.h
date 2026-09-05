#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"

class EnemySpawnEffect : public GameEngine::IGameObject {
public:

	enum class Phase {
		kIn,
		kEnd,
	};

public:
	EnemySpawnEffect(GameEngine::Model* model, GameEngine::Model* planeModel, GameEngine::Model* waveModel, uint32_t outGH, uint32_t waveGH);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	// 基準位置
	GameEngine::WorldTransform baseWorld_;

private:

	float kInMaxTime_ = 2.0f;
	float kEndMaxTime_ = 2.0f;

	float kInStartPos_ = 50.0f;
	float kInEndScale_ = 1.0f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ビーム
	GameEngine::ModelComponent beamModel_;

	// 下地
	GameEngine::ModelComponent basePlaneModel_;

	// 外側
	GameEngine::ModelComponent outPlaneModel_;
	Transform outUvtransform_ = { {1.0f,1.0f,1.0f},{},{} };

	// ウェーブ
	GameEngine::ModelComponent waveModel_;
	Transform waveUvtransform_ = { {1.0f,1.0f,1.0f},{},{} };

	Phase phase_ = Phase::kIn;

	float timer_ = 0.0f;

private:

	// 登録する
	void Register();
};
