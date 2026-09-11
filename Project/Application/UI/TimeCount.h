#pragma once
#include "IGameObject.h"
#include "Sprite.h"
#include "DebugParameter.h"

/// <summary>
/// 3,2,1のカウントの数字
/// </summary>
class TimeCountUI : public GameEngine::IGameObject {
public:
	TimeCountUI(uint32_t oneGH,uint32_t twoGH,uint32_t threeGH);
	~TimeCountUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void SetRemainingTime(float remainingTime) { remainingTime_ = remainingTime; }

private:

	// 残り時間
	float remainingTime_ = 0.0f;
	// 表示中か
	bool isVisible_ = false;

	// 通常時の大きさ
	Vector2 baseScale_ = { 256.0f,256.0f };
	// 数字が切り替わった瞬間の拡大率
	float popRate_ = 1.8f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	uint32_t oneGH_ = 0;
	uint32_t twoGH_ = 0;
	uint32_t threeGH_ = 0;

	// 数字
	GameEngine::Sprite numSprite_;

	float timer_ = 1.0f;
};