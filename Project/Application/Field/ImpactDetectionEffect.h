#pragma once
#include "IGameObject.h"
#include "Model.h"
#include "WorldTransforms.h"
#include "DebugParameter.h"
#include <vector>
#include <memory>

class ImpactDetectionEffect : public GameEngine::IGameObject {
public:

	struct ParticleData {
		Transform transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }; // srt要素
		Vector4 color = {1.0f,1.0f,1.0f,1.0f};  // 色
		uint32_t textureHandle = 0; // テクスチャ
		Vector3 basePos = { 0.0f,0.0f,0.0f };   // 基準位置
	};

	// 衝撃
	struct ImpactSource {
		Vector3 pos = { 0.0f,0.0f,0.0f };  // 発生した位置
		float power = 1.0f;                     // 力
		float elapsedTime = 0.0f;               // 発生してからの経過時間
	};

public:
	ImpactDetectionEffect(GameEngine::Model* model, uint32_t texture);
	~ImpactDetectionEffect() = default;

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

public:

	/// <summary>
	/// 衝撃を与える
	/// </summary>
	/// <param name="pos">座標</param>
	/// <param name="power">力</param>
	void ApplayImpact(Vector3 pos, float power);

private:

	// 列の長さ
	uint32_t row_ = 20;

	// パーティクルの最大数
	uint32_t maxNum_ = row_ * row_;

	// 間隔
	float spacing_ = 2.5f;

	// 格子全体の中心
	Vector3 gridCenter_ = { 0.0f,0.0f,0.0f };

private:

	// 波が広がる速さ
	float waveSpeed_ = 22.0f;

	// 波のうねり
	float waveWidth_ = 8.0f;

	// 波の周期
	float waveCount_ = 1.0f;

	// 力が1の時に持ち上がる高さ
	float waveHeight_ = 3.0f;

	// 波の影響範囲
	float fadeDistance_ = 30.0f;

	// 色が最も濃くなる高さ
	float colorHeight_ = 2.0f;

	// 波が来た所を大きくする割合
	float scalePunch_ = 1.5f;

	// 波が無い時の大きさ
	float baseScale_ = 0.6f;

	// 通常色
	Vector4 idleColor_ = { 0.02f,0.04f,0.08f,0.08f };
	// 山の色
	Vector4 crestColor_ = { 1.0f,0.55f,0.15f,1.0f };
	// 谷の色
	Vector4 troughColor_ = { 0.1f,0.35f,1.0f,1.0f };

	// 同時に扱える衝撃の数
	size_t maxImpactNum_ = 32;

	// 発生中の衝撃
	std::vector<ImpactSource> impacts_;

private:
	GameEngine::Model* model_ = nullptr;
	GameEngine::WorldTransforms worldTransforms_;

	uint32_t textureGH_ = 0;

	std::vector<ParticleData> particles_;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

private:

	/// <summary>
	/// 1つの衝撃が格子点に与えるy軸の変位を求める
	/// </summary>
	/// <param name="source">衝撃の発生源</param>
	/// <param name="particlePos">格子点の位置</param>
	/// <returns>y軸の変位</returns>
	float EvaluateWave(const ImpactSource& source, const Vector3& particlePos) const;

	// 格子状に粒を並べ直す
	void ResetGrid();
};
