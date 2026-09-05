#include "TitleLogo.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <string>

#include "EasingManager.h"
#include "FPSCounter.h"
#include "Model.h"
#include "ModelComponent.h"
#include "ModelManager.h"
#include "RandomGenerator.h"
#include "RenderQueue.h"

using namespace GameEngine;

TitleLogo::TitleLogo(ModelManager* modelManager) {
	assert(modelManager);

	constexpr float kScale = 0.75f;
	constexpr float kPartSpacing = 1.9f;
	constexpr float kFirstPartX = -2.85f;
	constexpr float kLogoY = 1.7f;
	constexpr float kLogoZ = -5.0f;
	const Vector3 rotation = {
		std::numbers::pi_v<float> * 0.5f,
		std::numbers::pi_v<float>,
		0.0f
	};

	// 4文字をまとめる下地を、文字より少し奥に配置する。
	Model* bottomModel = modelManager->GetNameByModel("bottom.obj");
	assert(bottomModel && "Title logo model bottom.obj must be loaded.");
	if (bottomModel) {
		// bottom.objが使用する全マテリアルのライティングを無効化する。
		bottomModel->SetDefaultIsEnableLight(false, "TitleLogo");
		bottomModel->SetDefaultIsEnableLight(false, "Material.001");
		bottom_ = std::make_unique<ModelComponent>(bottomModel);
		bottom_->worldTransform_.Initialize({
			{1.0f, 1.0f, 1.0f},
			rotation,
			{0.0f, kLogoY, kLogoZ + 0.1f}
		});
	}

	for (std::size_t i = 0; i < parts_.size(); ++i) {
		const std::string modelName = "t" + std::to_string(i) + ".obj";
		Model* model = modelManager->GetNameByModel(modelName);
		assert(model && "Title logo models t0.obj through t3.obj must be loaded.");
		if (!model) {
			continue;
		}

		model->SetDefaultIsEnableLight(false);
		parts_[i] = std::make_unique<ModelComponent>(model);
		parts_[i]->worldTransform_.Initialize({
			{kScale, kScale, kScale},
			rotation,
			{kFirstPartX + kPartSpacing * static_cast<float>(i), kLogoY, kLogoZ}
		});
	}

	// ImGuiから各モデルのSRTを個別に操作できるよう登録する。
	auto registerSrt = [this](const std::string& groupName, ModelComponent& component) {
		Transform& transform = component.worldTransform_.transform_;
		debugParameter_.Register("Scale", transform.scale, 0, groupName);
		debugParameter_.Register("Rotate", transform.rotate, 1, groupName);
		debugParameter_.Register("Translate", transform.translate, 2, groupName);
		};

	if (bottom_) {
		registerSrt("Bottom", *bottom_);
	}
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			registerSrt("T" + std::to_string(i), *parts_[i]);
		}
	}

	// タイトル開始演出の調整値をImGuiへ登録する。
	debugParameter_.Register("FallDuration", fallDuration_, 0, "EntranceAnimation");
	debugParameter_.Register("FallInterval", fallInterval_, 1, "EntranceAnimation");
	debugParameter_.Register("FallStartOffsetY", fallStartOffsetY_, 2, "EntranceAnimation");
	debugParameter_.Register("BottomFadeDuration", bottomFadeDuration_, 3, "EntranceAnimation");

	// 入力待ち中に、左の文字から順番に跳ねるループ演出の調整値を登録する。
	debugParameter_.Register("HopDuration", idleHopDuration_, 0, "IdleAnimation");
	debugParameter_.Register("Interval", idleInterval_, 1, "IdleAnimation");
	debugParameter_.Register("LoopDelay", idleLoopDelay_, 2, "IdleAnimation");
	debugParameter_.Register("HopHeight", idleHopHeight_, 3, "IdleAnimation");
	debugParameter_.Register("ScaleAmount", idleScaleAmount_, 4, "IdleAnimation");
	debugParameter_.Register("RockAngle", idleRockAngle_, 5, "IdleAnimation");
	debugParameter_.Register("BottomCycleDuration", idleBottomCycleDuration_, 6, "IdleAnimation");
	debugParameter_.Register("BottomMoveAmplitude", idleBottomMoveAmplitude_, 7, "IdleAnimation");
	debugParameter_.Register("BottomScaleAmount", idleBottomScaleAmount_, 8, "IdleAnimation");

	// タイトル終了演出の調整値をImGuiへ登録する。
	debugParameter_.Register("ShakeDuration", shakeDuration_, 0, "Animation");
	debugParameter_.Register("ShakeStartAmplitude", shakeStartAmplitude_, 1, "Animation");
	debugParameter_.Register("ShakeEndAmplitude", shakeEndAmplitude_, 2, "Animation");
	debugParameter_.Register("ShakeFrequencyX", shakeFrequencyX_, 3, "Animation");
	debugParameter_.Register("ShakeFrequencyY", shakeFrequencyY_, 4, "Animation");
	debugParameter_.Register("MoveDuration", moveDuration_, 5, "Animation");
	debugParameter_.Register("MoveInterval", moveInterval_, 6, "Animation");
	debugParameter_.Register("MoveShakeAmplitude", moveShakeAmplitude_, 7, "Animation");
	debugParameter_.Register("MoveRotationSpeed", moveRotationSpeed_, 8, "Animation");
	debugParameter_.Register("MoveDistance", moveDistance_, 9, "Animation");
	debugParameter_.Register("BottomScalingDuration", bottomScalingDuration_, 10, "Animation");
	debugParameter_.Register("BottomScalingStart", bottomScalingStart_, 11, "Animation");
	debugParameter_.Register("BottomScalingEnd", bottomScalingEnd_, 12, "Animation");
	debugParameter_.Apply();
}

TitleLogo::~TitleLogo() = default;

void TitleLogo::AnimationStart() {
	if (animationState_ != AnimationState::Idle) {
		return;
	}

	// Idle演出のオフセットを外し、Decision時点の設定値を終了演出の基準姿勢にする。
	RestoreIdleTransforms();
	debugParameter_.ApplyIfDirty();
	CaptureAnimationOrigins();
	animationState_ = AnimationState::Shaking;
	shakeTimer_.Start(shakeDuration_, false);
	bottomScalingTimer_.Start(bottomScalingDuration_, false);
}

void TitleLogo::ResetAnimation() {
	debugParameter_.Apply();
	CaptureAnimationOrigins();
	animationState_ = AnimationState::Falling;
	fallTimer_.Start(GetFallSequenceDuration(), false);
	bottomFadeTimer_.Reset();
	shakeTimer_.Reset();
	moveTimer_.Reset();
	bottomScalingTimer_.Reset();
	idleElapsedTime_ = 0.0f;
	idleBottomElapsedTime_ = 0.0f;

	// t0～t3を画面上側の待機位置へ移動し、Bottomは透明にしておく。
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			parts_[i]->worldTransform_.transform_.translate =
				partAnimationOrigins_[i] + Vector3{ 0.0f, fallStartOffsetY_, 0.0f };
		}
	}
	if (bottom_) {
		bottom_->SetAlpha(0.0f);
	}
	UpdateTransforms();
}

bool TitleLogo::IsAnimationFinished() const {
	return animationState_ == AnimationState::Finished;
}

void TitleLogo::Update() {
	// Idle中の見た目を基準姿勢へ戻してから設定変更を反映し、値の累積を防ぐ。
	if (animationState_ == AnimationState::Idle) {
		RestoreIdleTransforms();
		if (debugParameter_.ApplyIfDirty()) {
			CaptureAnimationOrigins();
		}
	} else {
		debugParameter_.ApplyIfDirty();
	}
	UpdateAnimation(FpsCounter::deltaTime);
	UpdateTransforms();
}

void TitleLogo::DebugUpdate() {
	if (animationState_ == AnimationState::Idle) {
		RestoreIdleTransforms();
		if (debugParameter_.ApplyIfDirty()) {
			CaptureAnimationOrigins();
		}
	} else {
		debugParameter_.ApplyIfDirty();
	}
	UpdateTransforms();
}

void TitleLogo::UpdateAnimation(float deltaTime) {
	switch (animationState_) {
	case AnimationState::Falling: {
		fallTimer_.SetDuration(GetFallSequenceDuration());
		fallTimer_.Update(deltaTime);

		const float elapsed = fallTimer_.GetElapsedTime();
		const float partDuration = (std::max)(fallDuration_, 0.0f);
		const float interval = (std::max)(fallInterval_, 0.0f);
		for (std::size_t i = 0; i < parts_.size(); ++i) {
			if (!parts_[i]) {
				continue;
			}

			// t0からt3へ順に、上側の待機位置から本来の位置まで落下させる。
			const float localElapsed = elapsed - interval * static_cast<float>(i);
			const float progress = partDuration > 0.0f
				? std::clamp(localElapsed / partDuration, 0.0f, 1.0f)
				: (localElapsed >= 0.0f ? 1.0f : 0.0f);
			const float easedProgress = Apply(progress, EaseType::kEaseOutBounce);
			parts_[i]->worldTransform_.transform_.translate =
				partAnimationOrigins_[i] + Vector3{
					0.0f,
					fallStartOffsetY_ * (1.0f - easedProgress),
					0.0f
				};
		}

		if (fallTimer_.IsFinished()) {
			RestorePartTransforms();
			animationState_ = AnimationState::FadingBottom;
			bottomFadeTimer_.Start(bottomFadeDuration_, false);
		}
		return;
	}

	case AnimationState::FadingBottom:
		bottomFadeTimer_.SetDuration(bottomFadeDuration_);
		bottomFadeTimer_.Update(deltaTime);
		if (bottom_) {
			bottom_->SetAlpha(Lerp(0.0f, 1.0f, bottomFadeTimer_.GetProgress()));
		}
		if (bottomFadeTimer_.IsFinished()) {
			if (bottom_) {
				bottom_->SetAlpha(1.0f);
			}
			idleElapsedTime_ = 0.0f;
			idleBottomElapsedTime_ = 0.0f;
			animationState_ = AnimationState::Idle;
		}
		return;

	case AnimationState::Idle: {
		// Bottomは文字のウェーブとは別周期で、常にゆっくり上下・拡縮させる。
		const float bottomCycleDuration = (std::max)(idleBottomCycleDuration_, 0.0f);
		if (bottom_ && bottomCycleDuration > 0.0f) {
			idleBottomElapsedTime_ += (std::max)(deltaTime, 0.0f);
			idleBottomElapsedTime_ = std::fmod(idleBottomElapsedTime_, bottomCycleDuration);
			const float phase = std::numbers::pi_v<float> * 2.0f
				* idleBottomElapsedTime_ / bottomCycleDuration;
			const float wave = std::sin(phase);

			bottom_->worldTransform_.transform_.translate =
				bottomAnimationOrigin_ + Vector3{ 0.0f, idleBottomMoveAmplitude_ * wave, 0.0f };
			bottom_->worldTransform_.transform_.scale =
				bottomAnimationOriginScale_ * (1.0f + idleBottomScaleAmount_ * wave);
		}

		const float hopDuration = (std::max)(idleHopDuration_, 0.0f);
		const float interval = (std::max)(idleInterval_, 0.0f);
		const float sequenceDuration = hopDuration + interval * static_cast<float>(kPartCount - 1);
		const float loopDuration = sequenceDuration + (std::max)(idleLoopDelay_, 0.0f);

		if (hopDuration <= 0.0f || loopDuration <= 0.0f) {
			RestorePartTransforms();
			return;
		}

		idleElapsedTime_ += (std::max)(deltaTime, 0.0f);
		idleElapsedTime_ = std::fmod(idleElapsedTime_, loopDuration);

		for (std::size_t i = 0; i < parts_.size(); ++i) {
			if (!parts_[i]) {
				continue;
			}

			const float localElapsed = idleElapsedTime_ - interval * static_cast<float>(i);
			if (localElapsed < 0.0f || localElapsed >= hopDuration) {
				continue;
			}

			const float progress = std::clamp(localElapsed / hopDuration, 0.0f, 1.0f);
			// 上昇は軽く、着地は少し速めにして、左から右へ弾むウェーブを作る。
			const float hopProgress = progress < 0.45f
				? Apply(progress / 0.45f, EaseType::kEaseOutCubic)
				: 1.0f - Apply((progress - 0.45f) / 0.55f, EaseType::kEaseInQuad);
			const float scale = 1.0f + idleScaleAmount_ * std::sin(std::numbers::pi_v<float> * progress);
			const float rock = idleRockAngle_
				* std::sin(std::numbers::pi_v<float> * 2.0f * progress)
				* hopProgress;

			parts_[i]->worldTransform_.transform_.translate =
				partAnimationOrigins_[i] + Vector3{ 0.0f, idleHopHeight_ * hopProgress, 0.0f };
			parts_[i]->worldTransform_.transform_.scale = partAnimationOriginScales_[i] * scale;
			parts_[i]->worldTransform_.transform_.rotate =
				partAnimationOriginRotations_[i] + Vector3{ 0.0f, 0.0f, rock };
		}
		return;
	}

	case AnimationState::Finished:
		return;

	case AnimationState::Shaking: {
		// ImGuiで実行中に変更された時間も現在のタイマーへ反映する。
		shakeTimer_.SetDuration(shakeDuration_);
		shakeTimer_.Update(deltaTime);

		if (shakeTimer_.IsFinished()) {
			RestorePartTransforms();
			RandomizeMoveRotationDirections();
			animationState_ = AnimationState::Moving;
			moveTimer_.Start(GetMoveSequenceDuration(), false);
			return;
		}

		// シェイクの進行に合わせて、開始時から終了時の強さへ徐々に変化させる。
		const float strength = Lerp(
			shakeStartAmplitude_,
			shakeEndAmplitude_,
			shakeTimer_.GetProgress()
		);
		const float elapsed = shakeTimer_.GetElapsedTime();
		for (std::size_t i = 0; i < parts_.size(); ++i) {
			if (!parts_[i]) {
				continue;
			}

			const float phase = static_cast<float>(i) * 1.7f;
			parts_[i]->worldTransform_.transform_.translate =
				partAnimationOrigins_[i] + Vector3{
					std::sin(elapsed * shakeFrequencyX_ + phase) * strength,
					std::cos(elapsed * shakeFrequencyY_ + phase) * strength,
					0.0f
				};
		}

		if (bottomScalingTimer_.IsFinished()) {
			bottom_->worldTransform_.transform_.scale = Vector3({0.0f,0.0f,0.0f});
		}
		else
		{
			Vector3 scale = GameEngine::Lerp(bottomScalingStart_, bottomScalingEnd_, bottomScalingTimer_.GetProgress(), EaseType::kLinear);
			bottom_->worldTransform_.transform_.scale = scale;
		}

		bottomScalingTimer_.Update(deltaTime);
		return;
	}

	case AnimationState::Moving: {
		moveTimer_.SetDuration(GetMoveSequenceDuration());
		moveTimer_.Update(deltaTime);
		const float sequenceProgress = moveTimer_.GetProgress();
		const float elapsed = moveTimer_.GetElapsedTime();
		const float partDuration = (std::max)(moveDuration_, 0.0f);
		const float interval = (std::max)(moveInterval_, 0.0f);

		// bottomは演出全体を通して奥へ移動する。
		if (bottom_) {
			const float bottomProgress = Apply(sequenceProgress, EaseType::kEaseInCubic);
			const Vector3 bottomOffset = {0.0f, 0.0f, moveDistance_ * bottomProgress};
			bottom_->worldTransform_.transform_.translate = bottomAnimationOrigin_ + bottomOffset;
		}

		// MoveInterval秒ずつ開始を遅らせ、t0からt3まで順番に奥へ移動する。
		for (std::size_t i = 0; i < parts_.size(); ++i) {
			if (parts_[i]) {
				const float localElapsed = elapsed - interval * static_cast<float>(i);
				const float partProgress = partDuration > 0.0f
					? std::clamp(localElapsed / partDuration, 0.0f, 1.0f)
					: (localElapsed >= 0.0f ? 1.0f : 0.0f);
				const float easedProgress = Apply(partProgress, EaseType::kEaseInQuart);
				const Vector3 partOffset = {0.0f, 0.0f, moveDistance_ * easedProgress};

				// この文字が移動している間だけ、固定強度でXYZ方向へ揺らす。
				Vector3 shakeOffset{};
				if (localElapsed >= 0.0f && localElapsed < partDuration) {
					const float phase = static_cast<float>(i) * 1.7f;
					shakeOffset = {
						std::sin(localElapsed * shakeFrequencyX_ + phase) * moveShakeAmplitude_,
						std::cos(localElapsed * shakeFrequencyY_ + phase) * moveShakeAmplitude_,
						std::sin(localElapsed * (shakeFrequencyX_ + shakeFrequencyY_) * 0.5f + phase) * moveShakeAmplitude_
					};
				}
				parts_[i]->worldTransform_.transform_.translate =
					partAnimationOrigins_[i] + partOffset + shakeOffset;

				// 移動開始から完了まで、各軸にランダムな方向で回転を加える。
				const float rotationTime = std::clamp(localElapsed, 0.0f, partDuration);
				parts_[i]->worldTransform_.transform_.rotate =
					partAnimationOriginRotations_[i] +
					partMoveRotationDirections_[i] * (moveRotationSpeed_ * rotationTime);
			}
		}

		if (moveTimer_.IsFinished()) {
			animationState_ = AnimationState::Finished;
		}
		return;
	}
	}
}

float TitleLogo::GetFallSequenceDuration() const {
	const float partDuration = (std::max)(fallDuration_, 0.0f);
	const float interval = (std::max)(fallInterval_, 0.0f);
	return partDuration + interval * static_cast<float>(kPartCount - 1);
}

float TitleLogo::GetMoveSequenceDuration() const {
	const float partDuration = (std::max)(moveDuration_, 0.0f);
	const float interval = (std::max)(moveInterval_, 0.0f);
	return partDuration + interval * static_cast<float>(kPartCount - 1);
}

void TitleLogo::UpdateTransforms() {
	if (bottom_) {
		bottom_->Update();
	}

	for (const auto& part : parts_) {
		if (part) {
			part->Update();
		}
	}
}

void TitleLogo::CaptureAnimationOrigins() {
	if (bottom_) {
		bottomAnimationOrigin_ = bottom_->worldTransform_.transform_.translate;
		bottomAnimationOriginScale_ = bottom_->worldTransform_.transform_.scale;
	}
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			partAnimationOrigins_[i] = parts_[i]->worldTransform_.transform_.translate;
			partAnimationOriginScales_[i] = parts_[i]->worldTransform_.transform_.scale;
			partAnimationOriginRotations_[i] = parts_[i]->worldTransform_.transform_.rotate;
		}
	}
}

void TitleLogo::RestorePartTransforms() {
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			parts_[i]->worldTransform_.transform_.translate = partAnimationOrigins_[i];
			parts_[i]->worldTransform_.transform_.scale = partAnimationOriginScales_[i];
			parts_[i]->worldTransform_.transform_.rotate = partAnimationOriginRotations_[i];
		}
	}
}

void TitleLogo::RestoreIdleTransforms() {
	RestorePartTransforms();
	if (bottom_) {
		bottom_->worldTransform_.transform_.translate = bottomAnimationOrigin_;
		bottom_->worldTransform_.transform_.scale = bottomAnimationOriginScale_;
	}
}

void TitleLogo::RandomizeMoveRotationDirections() {
	for (auto& direction : partMoveRotationDirections_) {
		direction = {
			RandomGenerator::Get<int>(0, 1) == 0 ? -1.0f : 1.0f,
			RandomGenerator::Get<int>(0, 1) == 0 ? -1.0f : 1.0f,
			RandomGenerator::Get<int>(0, 1) == 0 ? -1.0f : 1.0f
		};
	}
}

void TitleLogo::Draw(RenderQueue* renderQueue) {
	if (!renderQueue) {
		return;
	}

	if (bottom_) {
		bottom_->Draw(renderQueue);
	}

	for (const auto& part : parts_) {
		if (part) {
			part->Draw(renderQueue);
		}
	}
}
