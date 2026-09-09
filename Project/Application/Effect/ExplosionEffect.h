#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"

namespace GameEngine {
	class TextureManager;
	class ModelManager;
	class GameObjectManager;
	class ParticleBehavior;
}

/// <summary>
/// 爆発エフェクト
/// </summary>
class ExplosionEffect : public GameEngine::IGameObject {
public:
	ExplosionEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	/// <summary>
	/// 指定した位置で爆発を再生する
	/// </summary>
	/// <param name="pos">爆発の中心位置</param>
	void Start(Vector3 pos);

	// 再生中か
	bool IsPlay() const { return isPlay_; }

private:

	// フラッシュ
	float flashScaleTime_ = 0.10f;   // 最大まで広がる時間
	float flashFadeTime_ = 0.20f;    // 消えるまでの時間
	float startFlashScale_ = 0.5f;
	float endFlashScale_ = 8.0f;
	Vector4 flashColor_ = { 1.0f,0.95f,0.8f,1.0f };

	// リング
	float ringDelayTime_ = 0.06f;    // フラッシュの後に出す為の遅延
	float ringSpreadTime_ = 0.55f;   // 広がりきる時間
	float startRingScale_ = 1.0f;
	float endRingScale_ = 14.0f;
	Vector4 ringColor_ = { 1.0f,0.7f,0.35f,1.0f };

	// 爆発パーティクル
	float particleDelayTime_ = 0.05f; // 発生させるまでの遅延
	float playMaxTime_ = 2.0f;        // エフェクト全体の再生時間

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{ "ExplosionEffect" };

	// 爆発
	GameEngine::ParticleBehavior * explosionRingParticle_;
	GameEngine::ParticleBehavior* explosionParticle_;
	GameEngine::ParticleBehavior* explosionLineParticle_;

	// リング
	GameEngine::ModelComponent ringModel_;

	// フラッシュ
	GameEngine::ModelComponent flashModel_;

	// 再生管理
	bool isPlay_ = false;
	bool isEmit_ = false;
	float timer_ = 0.0f;

	// 発生位置
	Vector3 emitPos_ = { 0.0f,0.0f,0.0f };

private:

	// 登録する
	void Register();

	/// <summary>
	/// 遅延と長さから進行度(0.0～1.0)を求める
	/// </summary>
	float GetProgress(float delayTime, float maxTime) const;

	// ビルボード行列を更新する
	void UpdateMatrix();
};
