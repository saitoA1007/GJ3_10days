#include "FieldEffect.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

namespace {

	// 座標から0.0~1.0の乱数を作る
	float Rand(float x, float y) {
		float value = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
		return value - std::floor(value);
	}

	// 0~1のなめらかな補間係数にする
	float SmoothStep(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// 色の線形補間
	Vector4 LerpColor(const Vector4& start, const Vector4& end, float t) {
		return {
			start.x + (end.x - start.x) * t,
			start.y + (end.y - start.y) * t,
			start.z + (end.z - start.z) * t,
			start.w + (end.w - start.w) * t };
	}
}

FieldEffect::FieldEffect(GameEngine::Model* model, uint32_t texture) {
	model_ = model;
	textureGH_ = texture;

	// メモリ確保
	particles_.resize(maxNum_);
	waves_.reserve(maxWaveNum_);
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
	debugParame_->Register("color", color_);
	debugParame_->Register("wavePropagateSpeed", wavePropagateSpeed_);
	debugParame_->Register("waveBandWidth", waveBandWidth_);
	debugParame_->Register("waveHoldWidth", waveHoldWidth_);
	debugParame_->Register("waveFadeWidth", waveFadeWidth_);
	debugParame_->Register("wavePopHeight", wavePopHeight_);
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

	// フレームレートに依存しない補間の割合
	const float rate = 1.0f - std::exp(-followRate_ * deltaTime);

	// 波を外側へ広げる。一番遠いcubeが元の色へ戻りきった波は消す
	const float waveTotalWidth = waveBandWidth_ + waveHoldWidth_ + waveFadeWidth_;
	for (size_t i = 0; i < waves_.size();) {
		waves_[i].radius += wavePropagateSpeed_ * deltaTime;

		if (waves_[i].radius >= waves_[i].maxDist + waveTotalWidth) {
			// 消える時点で色の影響は0になっているので、途中で切れて見えることはない
			waves_.erase(waves_.begin() + i);
		} else {
			++i;
		}
	}

	for (uint32_t i = 0; i < activeNum_; ++i) {
		ParticleData& particle = particles_[i];

		// xz平面上での目標位置との距離を求める
		float diffX = particle.basePos.x - targetPos_.x;
		float diffZ = particle.basePos.z - targetPos_.z;
		float distance = std::sqrt(diffX * diffX + diffZ * diffZ);

		// 近いほど1.0、influenceRadius_より遠いと0.0になる係数。なめらかな山なりにする
		float t = SmoothStep(1.0f - (distance / influenceRadius_));

		// 土台の高さ。目標に近いほど高くなる
		float targetHeight = minHeight_ + (maxHeight_ - minHeight_) * t;

		// 土台の高さだけを補間する。波は補間すると振幅が痩せるので後から足す
		particle.height += (targetHeight - particle.height) * rate;

		// 遠いcubeほど少し速く揺らして、動きに差を作る
		float waveSpeed = waveSpeed_ * (1.0f + distance * waveSpeedByDist_);

		// 位相を積算する。sin(phase + time * speed)のようにspeedを時間へ掛けると、
		// プレイヤーが動いてspeedが変わった瞬間に位相が飛んで震えてしまう
		particle.phase += waveSpeed * deltaTime;
		// 精度が落ちないように0~2PIへ丸める
		if (particle.phase > TWO_PI) {
			particle.phase -= TWO_PI;
		}

		// 目標から遠くてもidleWaveHeight_の分だけは揺れ続ける
		float waveHeight = idleWaveHeight_ + (nearWaveHeight_ - idleWaveHeight_) * t;
		float wave = std::sin(particle.phase) * waveHeight;

		// 色の伝播。元の色から始めて、走っている波を古い順に重ねていく
		particle.color = color_;
		float pop = 0.0f;

		for (const ColorWave& colorWave : waves_) {

			// この波の中心からの距離
			float waveDiffX = particle.basePos.x - colorWave.origin.x;
			float waveDiffZ = particle.basePos.z - colorWave.origin.z;
			float waveDist = std::sqrt(waveDiffX * waveDiffX + waveDiffZ * waveDiffZ);

			// 0なら影響なし、1ならこの波の色に染まりきる
			float blend = CalcWaveBlend(colorWave.radius - waveDist);
			if (blend <= 0.0f) {
				continue;
			}

			// 後から始まった波ほど上に乗る
			particle.color = LerpColor(particle.color, colorWave.color, blend);

			// 色と同じ割合で持ち上げて、色の輪が地面を走っているように見せる
			// 重なった所で高く伸びすぎないように、一番強い波に合わせる
			pop = std::max(pop, blend * wavePopHeight_);
		}

		// 土台 + 揺れ + 波の跳ね。地面に潜らないように下限を設ける
		float height = std::max(particle.height + wave + pop, 0.01f);

		// cubeは原点中心・半径1なので、y方向のscaleは高さの半分になる
		particle.transform.scale = { cubeScale_, height * 0.5f, cubeScale_ };
		// 底面がcenter_.yに揃うように持ち上げる
		particle.transform.translate = { particle.basePos.x, center_.y + height * 0.5f, particle.basePos.z };

		// モデルに反映する
		cubeModels_[i]->worldTransform_.transform_ = particle.transform;
		cubeModels_[i]->materialData_->color = particle.color;
		cubeModels_[i]->Update();
	}
}

float FieldEffect::CalcWaveBlend(float passed) const {

	// passedは波の先端がそのcubeを追い越した距離。負なら波はまだ届いていない
	if (passed <= 0.0f) {
		return 0.0f;
	}

	// 色が乗っていく区間
	if (passed < waveBandWidth_) {
		return SmoothStep(passed / waveBandWidth_);
	}

	// 色が乗ったまま保たれる区間
	if (passed < waveBandWidth_ + waveHoldWidth_) {
		return 1.0f;
	}

	// 元の色へ戻っていく区間
	float fade = (passed - waveBandWidth_ - waveHoldWidth_) / waveFadeWidth_;
	return 1.0f - SmoothStep(fade);
}

void FieldEffect::Start(const Vector4& color) {

	ColorWave colorWave;
	// 呼ばれた瞬間のターゲット位置を波の中心にする
	colorWave.origin = targetPos_;
	colorWave.color = color;
	colorWave.radius = 0.0f;
	colorWave.maxDist = 0.0f;

	// 一番遠いcubeまでの距離を測っておく。ここまで届いたら波を消す
	for (uint32_t i = 0; i < activeNum_; ++i) {
		const Vector3& basePos = particles_[i].basePos;

		float diffX = basePos.x - colorWave.origin.x;
		float diffZ = basePos.z - colorWave.origin.z;

		colorWave.maxDist = std::max(colorWave.maxDist, std::sqrt(diffX * diffX + diffZ * diffZ));
	}

	// 連打されても増え続けないように、古い波から捨てる
	while (waves_.size() >= maxWaveNum_) {
		waves_.erase(waves_.begin());
	}

	waves_.push_back(colorWave);
}

void FieldEffect::DebugUpdate() {
#ifdef USE_IMGUI
	if (!ImGui::Begin("FieldEffect")) {
		ImGui::End();
		return;
	}

	ImGui::ColorEdit4("startColor", &debugStartColor_.x);
	if (ImGui::Button("Start")) {
		Start(debugStartColor_);
	}
	ImGui::Text("waveNum : %zu / %zu", waves_.size(), maxWaveNum_);
	for (size_t i = 0; i < waves_.size(); ++i) {
		ImGui::Text("  [%zu] radius %.2f / %.2f", i, waves_[i].radius, waves_[i].maxDist);
	}

	ImGui::End();
#endif
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

			// cubeごとに開始位相をずらして、揃って動かないようにする
			particle.phase = Rand(particle.basePos.x, particle.basePos.z) * TWO_PI;

			// 色は元の色でそろえる。走っている波はUpdate()で上に乗る
			particle.color = color_;

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
