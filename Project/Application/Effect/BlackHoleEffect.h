#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "BlackHoleMaterial.h"
#include "BlackHoleRingMaterial.h"

class BlackHoleEffect : public GameEngine::IGameObject {
public:
	BlackHoleEffect(GameEngine::Model* sphereModel, GameEngine::Model* ringModel);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	// 基準位置
	GameEngine::WorldTransform baseWorld_;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ブラックホール
	GameEngine::ModelComponent sphereModel_;
	GameEngine::BlackHoleMaterial blackHoleMaterial_;
	Vector4 glowColor_ = { 1.0f,1.0f,1.0f,1.0f };

	// リング
	GameEngine::ModelComponent ringModel_;
	GameEngine::BlackHoleRingMaterial blackHoleRingMaterial_;
	Vector4 ringGlowColor_ = { 1.0f,1.0f,1.0f,1.0f };

private:

	// 登録する
	void Register();
};