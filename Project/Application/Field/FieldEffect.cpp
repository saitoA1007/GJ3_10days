#define NOMINMAX
#include "FieldEffect.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

namespace {

	// 座標から0.0~1.0の乱数を作る(GLSLでよく使われるhash)
	float Rand(float x, float y) {
		float value = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
		return value - std::floor(value);
	}
}

FieldEffect::FieldEffect(GameEngine::Model* model, uint32_t texture) {
	model_ = model;
	textureGH_ = texture;

	// メモリ確保
	particles_.resize(maxNum_);
	cubeModels_.reserve(maxNum_);
	for (uint32_t i = 0; i < maxNum_; ++i) {
		std::unique_ptr<ModelComponent> cube = std::make_unique<ModelComponent>(model);
		cube->materialData_->color = { 0.0f,0.0f,0.0f,1.0f };
		cubeModels_.push_back(std::move(cube));
	}

	// パラメータの登録
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("FieldEffect");
	debugParame_->Register("ringNum", ringNum_);
	debugParame_->Register("cubeScale", cubeScale_);
	debugParame_->Register("gap", gap_);
	debugParame_->Register("center", center_);
	debugParame_->Register("influenceRadius", influenceRadius_);
	debugParame_->Register("minHeight", minHeight_);
	debugParame_->Register("maxHeight", maxHeight_);
	debugParame_->Register("followRate", followRate_);
	debugParame_->Register("waveSpeed", waveSpeed_);
	debugParame_->Register("waveSpeedByDist", waveSpeedByDist_);
	debugParame_->Register("idleWaveHeight", idleWaveHeight_);
	debugParame_->Register("nearWaveHeight", nearWaveHeight_);
	debugParame_->Apply();

	// 円状に並べる
	ResetCircle();

	Update();
}

void FieldEffect::Initialize() {

}

void FieldEffect::Update() {

	// 調整した値を反映する
	if (debugParame_->ApplyIfDirty()) {
		ResetCircle();
	}

	const float deltaTime = FpsCounter::gameDeltaTime;
	time_ += deltaTime;

	// フレームレートに依存しない補間の割合
	const float rate = 1.0f - std::exp(-followRate_ * deltaTime);

	for (uint32_t i = 0; i < activeNum_; ++i) {
		ParticleData& particle = particles_[i];

		// xz平面上での目標位置との距離を求める
		float diffX = particle.basePos.x - targetPos_.x;
		float diffZ = particle.basePos.z - targetPos_.z;
		float distance = std::sqrt(diffX * diffX + diffZ * diffZ);

		// 近いほど1.0、influenceRadius_より遠いと0.0になる係数
		float t = 1.0f - (distance / influenceRadius_);
		t = std::clamp(t, 0.0f, 1.0f);
		// なめらかな山なりの形にする
		t = t * t * (3.0f - 2.0f * t);

		// 土台の高さ。目標に近いほど高くなる
		float targetHeight = minHeight_ + (maxHeight_ - minHeight_) * t;

		// 土台の高さだけを補間する。波は補間すると振幅が痩せるので後から足す
		particle.height += (targetHeight - particle.height) * rate;

		// 遠いcubeほど少し速く揺らして、動きに差を作る
		float waveSpeed = waveSpeed_ * (1.0f + distance * waveSpeedByDist_);
		// 目標から遠くてもidleWaveHeight_の分だけは揺れ続ける
		float waveHeight = idleWaveHeight_ + (nearWaveHeight_ - idleWaveHeight_) * t;
		// cubeごとに位相をずらして、揃って動かないようにする
		float wave = std::sin(particle.phase + time_ * waveSpeed) * waveHeight;

		// 土台 + 揺れ。地面に潜らないように下限を設ける
		float height = std::max(particle.height + wave, 0.01f);

		// cubeは原点中心・半径1なので、y方向のscaleは高さの半分になる
		particle.transform.scale = { cubeScale_, height * 0.5f, cubeScale_ };
		// 底面がcenter_.yに揃うように持ち上げる
		particle.transform.translate = { particle.basePos.x, center_.y + height * 0.5f, particle.basePos.z };

		// モデルに反映する
		cubeModels_[i]->worldTransform_.transform_ = particle.transform;
		cubeModels_[i]->Update();
	}
}

void FieldEffect::DebugUpdate() {

}

void FieldEffect::Draw() {
	// 描画
	for (uint32_t i = 0; i < activeNum_; ++i) {
		cubeModels_[i]->DrawRaytracing(renderQueue_);
	}
}

void FieldEffect::ApplayPosition(Vector3 pos) {
	targetPos_ = pos;
}

void FieldEffect::ResetCircle() {

	// cube1個分の間隔
	const float step = cubeScale_ * 2.0f + gap_;

	activeNum_ = 0;

	// 中心から外側へ、1リングずつ並べていく
	for (uint32_t ring = 0; ring < ringNum_; ++ring) {

		// このリングの半径
		float ringRadius = step * static_cast<float>(ring);

		// 外側のリングほど多く並べて、cubeの間隔を一定に保つ
		uint32_t num = 1;
		if (ring > 0) {
			num = static_cast<uint32_t>(std::round(TWO_PI * ringRadius / step));
			num = std::max<uint32_t>(num, 1);
		}

		for (uint32_t i = 0; i < num; ++i) {

			// 確保した数を超えたら打ち切る
			if (activeNum_ >= maxNum_) {
				radius_ = ringRadius;
				return;
			}

			// リング上に等間隔で配置する
			float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(num);

			ParticleData& particle = particles_[activeNum_];
			particle.basePos = {
				center_.x + std::cos(theta) * ringRadius,
				center_.y,
				center_.z + std::sin(theta) * ringRadius };

			// cubeごとに固定のランダムな位相を持たせる
			particle.phase = Rand(particle.basePos.x, particle.basePos.z) * TWO_PI;

			particle.height = minHeight_;
			particle.transform.rotate = { 0.0f,0.0f,0.0f };
			particle.transform.scale = { cubeScale_, particle.height * 0.5f, cubeScale_ };
			particle.transform.translate = { particle.basePos.x, center_.y + particle.height * 0.5f, particle.basePos.z };

			++activeNum_;
		}
	}

	// 円全体の半径
	radius_ = step * static_cast<float>(ringNum_ - 1);
}
