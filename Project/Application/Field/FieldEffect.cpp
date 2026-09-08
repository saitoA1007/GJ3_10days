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

	// 波を外側へ広げる
	if (isPropagating_) {
		waveRadius_ += wavePropagateSpeed_ * deltaTime;
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

		// 色の伝播。波が通過する時に色を乗せ、通り過ぎたら元の色へ戻す
		float pop = 0.0f;
		if (isPropagating_) {

			// 波の先端がこのcubeをどれだけ追い越したか
			float passed = waveRadius_ - particle.waveDist;

			// 0で元の色、1で伝播色になる割合
			float blend = 0.0f;
			// 色が乗るまでは今の色から、戻る時は元の色へ向かって補間する
			Vector4 baseColor = color_;

			if (passed < waveBandWidth_) {
				// 色が乗っていく途中。伝播中に再度Start()されても繋がるように今の色から始める
				blend = SmoothStep(passed / waveBandWidth_);
				baseColor = particle.startColor;
			} else if (passed < waveBandWidth_ + waveHoldWidth_) {
				// 色が乗ったまま保たれる区間
				blend = 1.0f;
			} else {
				// 元の色へ戻っていく区間
				float fade = (passed - waveBandWidth_ - waveHoldWidth_) / waveFadeWidth_;
				blend = 1.0f - SmoothStep(fade);
			}

			particle.color = LerpColor(baseColor, waveColor_, blend);

			// 色と同じ割合で持ち上げて、色の輪が地面を走っているように見せる
			pop = blend * wavePopHeight_;
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

	// 一番遠いcubeが元の色へ戻りきったら終了する
	if (isPropagating_ && waveRadius_ >= waveMaxDist_ + waveBandWidth_ + waveHoldWidth_ + waveFadeWidth_) {
		isPropagating_ = false;
		// 端数を残さず、全てのcubeを元の色にそろえる
		for (uint32_t i = 0; i < activeNum_; ++i) {
			particles_[i].color = color_;
		}
	}
}

void FieldEffect::Start(const Vector4& color) {
	if (isPropagating_) { return; }
	isPropagating_ = true;

	// 呼ばれた瞬間のターゲット位置を波の中心にする
	waveOrigin_ = targetPos_;
	waveColor_ = color;
	waveRadius_ = 0.0f;
	waveMaxDist_ = 0.0f;

	for (uint32_t i = 0; i < activeNum_; ++i) {
		ParticleData& particle = particles_[i];

		// 伝播中にもう一度呼ばれても繋がるように、今の色から始める
		particle.startColor = particle.color;

		// xz平面上での中心からの距離。ここに波が届いた時に色が変わる
		float diffX = particle.basePos.x - waveOrigin_.x;
		float diffZ = particle.basePos.z - waveOrigin_.z;
		particle.waveDist = std::sqrt(diffX * diffX + diffZ * diffZ);

		waveMaxDist_ = std::max(waveMaxDist_, particle.waveDist);
	}
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
	ImGui::Text("propagating : %s", isPropagating_ ? "true" : "false");
	ImGui::Text("waveRadius  : %.2f / %.2f", waveRadius_, waveMaxDist_);

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

			// 色は現在の色でそろえる
			particle.color = color_;
			particle.startColor = color_;

			// 波の中心からの距離。並べ直しても伝播が続くように計算しておく
			float diffX = particle.basePos.x - waveOrigin_.x;
			float diffZ = particle.basePos.z - waveOrigin_.z;
			particle.waveDist = std::sqrt(diffX * diffX + diffZ * diffZ);

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
