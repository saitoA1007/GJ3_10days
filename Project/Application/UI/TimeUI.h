#pragma once
#include <vector>
#include "IGameObject.h"
#include "Sprite.h"
#include "DebugParameter.h"
#include "WorldTransform.h"

class TimeUI : public GameEngine::IGameObject {
public:
	TimeUI(uint32_t unitIceGH);
	~TimeUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// ユニットの位置を設定
	// 0.0f~1.0fでスタートからゴールまで移動する
	void SetUnit(float value) {
		unitT_ = value;
	}

private:

	float startPosX_ = 0.0f;
	float endPosX_ = 0.0f;

	Vector2 heightScale_ = { 1.0f,1.0f };
	Vector4 heightColor_ = { 1.0f,1.0f,1.0f,1.0f };

private:
	// パラメータ
	GameEngine::DebugParameter debugParame_{"TimeUI"};

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// ユニット
	GameEngine::Sprite unitSprite_;

	// 横のバー
	GameEngine::Sprite barSprite_;

	std::vector<GameEngine::Sprite> heightSprites_;

	float unitT_ = 0.0f;
};