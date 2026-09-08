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

class BlackHoleEffect;

class IncorporateEffect : public GameEngine::IGameObject {
public:

	enum class Phase {
		kIn, // 入りの演出
		kMain, // メイン演出
		kEnd, // 終了演出
	};

public:
	IncorporateEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	/// <summary>
	/// 演出を開始
	/// </summary>
	/// <param name="pos">位置</param>
	/// <param name="scale">ブラックホールの発生サイズ(1.0fで1m)</param>
	void Start(Vector3 pos, float scale);

private:

	// 入りの演出時間
	float kInMaxTime_ = 1.0f;
	// メインの演出時間
	float kMainMaxTime_ = 1.0f;
	// 終わりの演出時間
	float kEndMaxTime_ = 1.0f;

	// 入りのスター
	float starFadeTime_ = 0.20f;    // 消えるまでの時間

	// 入りのリング
	float ringDelayTime_ = 0.06f;    // フラッシュの後に出す為の遅延
	float ringSpreadTime_ = 0.55f;   // 広がりきる時間

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{ "IncorporateEffect" };

	// ブラックホール
	BlackHoleEffect* blackHoleEffect_ = nullptr;
	// ブラックホールの周りにあるエフェクト
	GameEngine::ParticleBehavior* blackHoleParticle_ = nullptr;

	// 入りで使用するスターパーティクル
	GameEngine::ParticleBehavior* starParticle_ = nullptr;
	// 入りで使用するリングパーティクル
	GameEngine::ParticleBehavior* ringParticle_ = nullptr;

	float endBlacHoleScale_ = 0.0f;

	// 現在のフェーズ
	Phase phase_ = Phase::kIn;

	// 再生管理
	bool isPlay_ = false;
	float timer_ = 0.0f;
	bool isEmitStar_ = false;
	bool isEmitRing_ = false;

	// 発生位置
	Vector3 emitPos_ = { 0.0f,0.0f,0.0f };

private:

	float GetProgress(float delayTime, float maxTime) const;
};