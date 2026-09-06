#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "BlackHoleRingMaterial.h"
#include "UniverseMaterial.h"

class SpawnFieldEffect : public GameEngine::IGameObject {
public:
	SpawnFieldEffect(GameEngine::Model* ring1Model, GameEngine::Model* ring2Model, GameEngine::Model* ring3Model, GameEngine::Model* halfDomeModel);

	void Initialize() override;
	void Update() override;
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// リング
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> ringModels_;
	std::vector<GameEngine::BlackHoleRingMaterial> materials_;
	std::vector<Vector4> glowColors_;

	// 下地のリング
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> underRingModels_;

	// 背景の宇宙
	GameEngine::ModelComponent universeModel_;
	GameEngine::UniverseMaterial universeMaterial_;

private:

	// 登録する
	void Register();
};