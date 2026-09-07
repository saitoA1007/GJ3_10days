#pragma once
#include <array>
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "BlackHoleRingMaterial.h"
#include "UniverseMaterial.h"

class SpawnFieldEffect : public GameEngine::IGameObject {
public:
	SpawnFieldEffect(GameEngine::Model* ring1Model, GameEngine::Model* ring2Model, GameEngine::Model* ring3Model,
		GameEngine::Model* halfDomeModel, GameEngine::Model* circleModel);

	void Initialize() override;
	void Update() override;
	void DebugUpdate() override;
	void Draw() override;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// リング
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> ringModels_;
	std::vector<GameEngine::BlackHoleRingMaterial> materials_;
	std::vector<Vector4> glowColors_;
	std::array<float, 3> ringModelScales_ = { 1.0f, 1.0f, 1.0f };

	// 下地のリング
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> underRingModels_;

	// 背景の宇宙
	GameEngine::ModelComponent universeModel_;
	GameEngine::UniverseMaterial universeMaterial_;

	// ステージの円
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> circleModels_;
	float fieldRadius_ = 20.0f;
	float height_ = 1.0f;
	// 円の色アニメーション
	float colorTime_ = 0.0f;   // 経過時間
	float colorCycle_ = 4.0f;   // 1周にかかる秒数
	float colorOffset_ = 0.125f; // リング1枚ごとの位相ずれ

private:

	// 登録する
	void Register();

	// Parameter Inspectorの変更をリングへ反映する
	void ApplyDebugParameters();

	// 発光リングと下地リングのXZスケールを揃えて更新する
	void ApplyRingModelScales();
};
