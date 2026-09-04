#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "DebugParameter.h"
#include "Vector3.h"
#include "../Utils/GameTimer.h"

namespace GameEngine {
	class ModelComponent;
	class ModelManager;
	class RenderQueue;
}

/// <summary>
/// タイトルロゴを構成するモデルをまとめて管理する。
/// </summary>
class TitleLogo final {
public:
	explicit TitleLogo(GameEngine::ModelManager* modelManager);
	~TitleLogo();

	void Update();
	void DebugUpdate();
	void Draw(GameEngine::RenderQueue* renderQueue);

	void AnimationStart();
	void ResetAnimation();
	bool IsAnimationFinished() const;

private:
	enum class AnimationState {
		Idle,
		Shaking,
		Moving,
		Finished,
	};

	void UpdateAnimation(float deltaTime);
	void UpdateTransforms();
	void CaptureAnimationOrigins();
	void RestorePartTranslations();
	float GetMoveSequenceDuration() const;

	static constexpr std::size_t kPartCount = 4;
	std::unique_ptr<GameEngine::ModelComponent> bottom_;
	std::array<std::unique_ptr<GameEngine::ModelComponent>, kPartCount> parts_;
	GameEngine::DebugParameter debugParameter_{ "TitleLogo" };

	float bottomScalingDuration_ = 1.0f;
	Vector3 bottomScalingStart_{ 1.0f, 1.0f };
	Vector3 bottomScalingEnd_{ 1.0f, 1.0f };

	float shakeDuration_ = 1.0f;
	float shakeStartAmplitude_ = 0.0f;
	float shakeEndAmplitude_ = 0.08f;
	float shakeFrequencyX_ = 55.0f;
	float shakeFrequencyY_ = 47.0f;
	float moveDuration_ = 1.25f;
	float moveInterval_ = 0.15f;
	float moveDistance_ = 180.0f;

	AnimationState animationState_ = AnimationState::Idle;
	GameTimer shakeTimer_;
	GameTimer bottomScalingTimer_;
	GameTimer moveTimer_;
	Vector3 bottomAnimationOrigin_{};
	std::array<Vector3, kPartCount> partAnimationOrigins_{};
};
