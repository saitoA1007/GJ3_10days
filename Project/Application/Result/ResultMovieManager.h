#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Application/UI/LetterboxUI.h"

class ResultMoveCamera;
class MoonObject;
class RocketEffect;
class ExplosionEffect;
class LetterBoxUI;

// リザルトのムービー演出を管理
class ResultMovieManager : public GameEngine::IGameObject {
public:
	ResultMovieManager(ResultMoveCamera* camera, MoonObject* moonObject, RocketEffect* rocketEffect, ExplosionEffect* explosionEffect);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start(float clearRate);

	bool IsFin() const;

private:
	// カメラ
	ResultMoveCamera* resultMoveCamera_ = nullptr;

	// 月
	MoonObject* moonObject_ = nullptr;

	// ロケットの演出
	RocketEffect* rocketEffect_ = nullptr;

	float clearRate_ = 0.0f;
	// 爆破演出
	ExplosionEffect* explosionEffect_ = nullptr;

	// 黒帯UI
	LetterboxUI letterboxUI_;

	bool isExplo_ = false;
};