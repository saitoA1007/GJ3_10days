#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "BlackHoleRingMaterial.h"

class SpawnFieldEffect : public GameEngine::IGameObject {
public:
	SpawnFieldEffect(GameEngine::Model* ring1Model, GameEngine::Model* ring2Model, GameEngine::Model* ring3Model);

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

	std::vector<std::unique_ptr<GameEngine::ModelComponent>> underRingModels_;

private:

	// 登録する
	void Register();
};