#pragma once
#include "IGameObject.h"
#include "Model.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include <vector>
#include <memory>

class FieldEffect : public GameEngine::IGameObject {
public:

	struct ParticleData {
		Transform transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // srt要素
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f };  // 色
		uint32_t textureHandle = 0; // テクスチャ
		Vector3 basePos = { 0.0f,0.0f,0.0f };   // 基準位置
		float height = 0.0f;                    // 現在の高さ
		float phase = 0.0f;                     // 揺れの位相
	};

	// 広がっていく色の波
	struct ColorWave {
		Vector3 origin = { 0.0f,0.0f,0.0f };      // 広がる中心
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f };  // 乗せる色
		float radius = 0.0f;                      // 現在の半径
		float maxDist = 0.0f;                     // 一番遠いcubeまでの距離
	};

public:
	FieldEffect(GameEngine::Model* model, uint32_t texture);
	~FieldEffect() = default;

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

public:

	// 高さを持ち上げる位置を設定
	void ApplayPosition(Vector3 pos);

	// 色を伝播させる演出を開始する
	void Start(const Vector4& color);

	// 伝播中かどうか
	bool IsPropagating() const { return !waves_.empty(); }

private:

	// リングの数(中心から外側に何重に並べるか)
	uint32_t ringNum_ = 20;

	// パーティクルの最大数。円状に並べるので リング数^2 * π 個ほど必要になる
	uint32_t maxNum_ = ringNum_ * ringNum_ * 4;

	// 全体の中心
	Vector3 center_ = { 0.0f,0.0f,0.0f };

	// cubeのxz方向の大きさ
	float cubeScale_ = 0.4f;

	// cube同士の隙間
	float gap_ = 0.2f;

	// 実際に並んでいる数
	uint32_t activeNum_ = 0;

	// 円全体の半径
	float radius_ = 20.0f;

private:

	// 高さを持ち上げる位置
	Vector3 targetPos_ = { 0.0f,0.0f,0.0f };

	// 影響が届く距離。これより遠いcubeはminHeight_のまま
	float influenceRadius_ = 8.0f;

	// 一番低いときの高さ
	float minHeight_ = 0.2f;

	// 一番高いときの高さ
	float maxHeight_ = 4.0f;

	// 目標の高さへの追従速度
	float followRate_ = 10.0f;

private:

	// 揺れの基準の速さ
	float waveSpeed_ = 2.0f;

	// 目標から遠いcubeほど揺れを速くする割合
	float waveSpeedByDist_ = 0.15f;

	// 目標から遠いcubeの揺れ幅。0にすると遠くのcubeは止まる
	float idleWaveHeight_ = 0.08f;

	// 目標に近いcubeの揺れ幅
	float nearWaveHeight_ = 0.5f;

	Vector4 color_ = { 0.0f,0.0f,0.0f,1.0f };

private:

	// 広がっている最中の波。後ろにあるものほど後から始まった波
	std::vector<ColorWave> waves_;

	// 同時に走らせる波の上限。超えた分は一番古い波を捨てる
	size_t maxWaveNum_ = 8;

	// 波の広がる速さ[単位/秒]
	float wavePropagateSpeed_ = 12.0f;

	// 色が乗るまでの境界の幅。大きいほどグラデーションが緩やかになる
	float waveBandWidth_ = 2.0f;

	// 色が乗ったまま保たれる帯の幅
	float waveHoldWidth_ = 2.0f;

	// 元の色へ戻るまでの帯の幅。ここを広くすると余韻が長く残る
	float waveFadeWidth_ = 6.0f;

	// 波が通過した瞬間に持ち上がる高さ
	float wavePopHeight_ = 1.5f;

	// デバッグ用の伝播させる色
	Vector4 debugStartColor_ = { 0.1f,0.6f,1.0f,1.0f };

private:
	GameEngine::Model* model_ = nullptr;

	uint32_t textureGH_ = 0;

	std::vector<ParticleData> particles_;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 足場のCube
	std::vector<std::unique_ptr<GameEngine::ModelComponent>> cubeModels_;

private:

	// 円状に粒を並べ直す
	void ResetCircle();

	// 波の先端が追い越した距離から、色をどれだけ乗せるかを求める
	float CalcWaveBlend(float passed) const;
};
