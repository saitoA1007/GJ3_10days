#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"

class NaviEffect : public GameEngine::IGameObject {
public:
	NaviEffect(GameEngine::Model* beamModel, uint32_t naviGH, GameEngine::Model* arrowModel);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start(Vector3 startPos, Vector3 endPos);

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ビーム
	GameEngine::ModelComponent beamModel_;
	Transform uvtransform_ = { {1.0f,1.0f,1.0f},{},{} };

	float timer_ = 0.0f;

	GameEngine::ModelComponent arrowModel_;

private:

	// 登録する
	void Register();
};