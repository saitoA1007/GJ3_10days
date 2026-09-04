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

	// タイトル終了演出の調整値をImGuiへ登録する。
	debugParameter_.Register("ShakeDuration", shakeDuration_, 0, "Animation");
	debugParameter_.Register("ShakeAmplitude", shakeAmplitude_, 1, "Animation");
	debugParameter_.Register("ShakeFrequencyX", shakeFrequencyX_, 2, "Animation");
	debugParameter_.Register("ShakeFrequencyY", shakeFrequencyY_, 3, "Animation");
	debugParameter_.Register("MoveDuration", moveDuration_, 4, "Animation");
	debugParameter_.Register("MoveDistance", moveDistance_, 5, "Animation");
	debugParameter_.Register("BottomScalingDuration", bottomScalingDuration_, 6, "Animation");
	debugParameter_.Register("BottomScalingStart", bottomScalingStart_, 7, "Animation");
	debugParameter_.Register("BottomScalingEnd", bottomScalingEnd_, 8, "Animation");
	debugParameter_.Apply();
}

TitleLogo::~TitleLogo() = default;

void TitleLogo::AnimationStart() {
	if (animationState_ != AnimationState::Idle) {
		return;
	}

	// Decision時点のImGui設定値を演出の基準位置として使用する。
	debugParameter_.ApplyIfDirty();
	CaptureAnimationOrigins();
	animationState_ = AnimationState::Shaking;
	shakeTimer_.Start(shakeDuration_, false);
	bottomScalingTimer_.Start(bottomScalingDuration_, false);
}

void TitleLogo::ResetAnimation() {
	debugParameter_.Apply();
	animationState_ = AnimationState::Idle;
	shakeTimer_.Reset();
	moveTimer_.Reset();
	bottomScalingTimer_.Reset();
	UpdateTransforms();
}

bool TitleLogo::IsAnimationFinished() const {
	return animationState_ == AnimationState::Finished;
}

void TitleLogo::Update() {
	debugParameter_.ApplyIfDirty();
	UpdateAnimation(FpsCounter::deltaTime);
	UpdateTransforms();
}

void TitleLogo::DebugUpdate() {
	debugParameter_.ApplyIfDirty();
	UpdateTransforms();
}

void TitleLogo::UpdateAnimation(float deltaTime) {
	switch (animationState_) {
	case AnimationState::Idle:
	case AnimationState::Finished:
		return;

	case AnimationState::Shaking: {
		// ImGuiで実行中に変更された時間も現在のタイマーへ反映する。
		shakeTimer_.SetDuration(shakeDuration_);
		shakeTimer_.Update(deltaTime);

		if (shakeTimer_.IsFinished()) {
			RestorePartTranslations();
			animationState_ = AnimationState::Moving;
			moveTimer_.Start(moveDuration_, false);
			return;
		}

		// 終端に近づくほど揺れ幅を小さくし、元の位置へ滑らかに戻す。
		const float strength = shakeAmplitude_ * shakeTimer_.GetReverseProgress();
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
		moveTimer_.SetDuration(moveDuration_);
		moveTimer_.Update(deltaTime);
		const float sequenceProgress = moveTimer_.GetProgress();

		// bottomは演出全体を通して奥へ移動する。
		if (bottom_) {
			const float bottomProgress = Apply(sequenceProgress, EaseType::kEaseInCubic);
			const Vector3 bottomOffset = {0.0f, 0.0f, moveDistance_ * bottomProgress};
			bottom_->worldTransform_.transform_.translate = bottomAnimationOrigin_ + bottomOffset;
		}

		// 移動時間を4分割し、t0からt3まで1つずつ順番に奥へ移動する。
		const float partCount = static_cast<float>(parts_.size());
		for (std::size_t i = 0; i < parts_.size(); ++i) {
			if (parts_[i]) {
				const float partProgress = std::clamp(
					sequenceProgress * partCount - static_cast<float>(i),
					0.0f,
					1.0f
				);
				const float easedProgress = Apply(partProgress, EaseType::kEaseInBack);
				const Vector3 partOffset = {0.0f, 0.0f, moveDistance_ * easedProgress};
				parts_[i]->worldTransform_.transform_.translate = partAnimationOrigins_[i] + partOffset;
			}
		}

		if (moveTimer_.IsFinished()) {
			animationState_ = AnimationState::Finished;
		}
		return;
	}
	}
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
	}
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			partAnimationOrigins_[i] = parts_[i]->worldTransform_.transform_.translate;
		}
	}
}

void TitleLogo::RestorePartTranslations() {
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			parts_[i]->worldTransform_.transform_.translate = partAnimationOrigins_[i];
		}
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
