#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "ParticleBehavior.h"

namespace GameEngine {
	class TextureManager;
	class ModelManager;
	class GameObjectManager;
}

class RocketEffect : public GameEngine::IGameObject {
public:
	enum class Phase {
		Idle,   // 待機
		Boost,  // 推進中
		Overrun,
		Fall,   // 落下中
	};

public:
	RocketEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start(float energy);

	void Reset() {
		auto& tr = rocketModel_.worldTransform_.transform_;
		tr.translate = startPos_;
		tr.rotate.z = 0.0f;
		isRocketStarted_ = false;
		timer_ = 0.0f;
	}

	float GetEnergy() const { return energy_; }

	Phase GetPhase() const { return phase_; }

public:

	//ロケット
	GameEngine::ModelComponent rocketModel_;

private:

	float kMoveMaxTime_ = 2.0f;

	Vector3 startPos_ = {0.0f,0.0f,0.0f};
	Vector3 endPos_ = {80.0f,50.0f,0.0f};
	
private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{"RocketEffect"};

	// 炎
	GameEngine::ParticleBehavior* fireEffect_ = nullptr;

	float timer_ = 0.0f;

	bool isActiveAnimation_ = false;

	// 補正
	float kRotateOffsetZ_ = 0.0f;

	// エネルギー
	float energy_ = 1.0f;

	Phase phase_ = Phase::Idle;

	// 落下用
	Vector3 velocity_ = { 0.0f,0.0f,0.0f };
	float kGravity_ = -60.0f;   // 落下加速度
	float kGroundY_ = -10.0f;     // 地面の高さ

	// 通過後の追加移動
	float kOverrunDistanceX_ = 20.0f;
	float overrunTimer_ = 0.0f;
	float overrunTime_ = 0.3f;

	bool isRocketStarted_ = false;

private:

	// 登録する
	void Register();

	void UpdateMove();

	void UpdateBoost();
	void UpdateOverrun();
	void UpdateFall();

	Vector3 CalcPos(float t) const;

	Vector3 CalcVelocity(float t) const;
};