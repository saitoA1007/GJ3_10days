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
		float height = 0.0f;                    // 現在の高さ(波を含まない土台部分)
		float phase = 0.0f;                     // 揺れの位相。cubeごとにばらけさせる
	};

public:
	FieldEffect(GameEngine::Model* model, uint32_t texture);
	~FieldEffect() = default;

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

public:

	// 高さを持ち上げる位置(プレイヤーの位置)を設定
	void ApplayPosition(Vector3 pos);

private:

	// リングの数(中心から外側に何重に並べるか)
	uint32_t ringNum_ = 20;

	// パーティクルの最大数。円状に並べるので リング数^2 * π 個ほど必要になる
	uint32_t maxNum_ = ringNum_ * ringNum_ * 4;

	// 全体の中心
	Vector3 center_ = { 0.0f,0.0f,0.0f };

	// cubeのxz方向の大きさ(cube.objは原点中心の半径1なので、実際の幅はこの2倍)
	float cubeScale_ = 0.4f;

	// cube同士の隙間
	float gap_ = 0.2f;

	// 実際に並んでいる数
	uint32_t activeNum_ = 0;

	// 円全体の半径(ResetCircleで計算される)
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

	// 経過時間
	float time_ = 0.0f;

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
};
