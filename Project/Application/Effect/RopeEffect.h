#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"

class RopeEffect : public GameEngine::IGameObject {
public:
	RopeEffect(GameEngine::Model* beamModel, uint32_t beamGH);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start(Vector3 startPos, Vector3 endPos);

	// 色を設定
	void SetColor(Vector4 color) {
		beamModel_.materialData_->color = color;
	}

	void SetScale(float scale) {
		beamModel_.worldTransform_.transform_.scale.x = scale;
		beamModel_.worldTransform_.transform_.scale.y = scale;
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// ビーム
	GameEngine::ModelComponent beamModel_;
	Transform uvtransform_ = { {1.0f,1.0f,1.0f},{},{} };
	float scrollSpeed_ = 0.0f;

	float timer_ = 0.0f;

private:

	// 登録する
	void Register();
};