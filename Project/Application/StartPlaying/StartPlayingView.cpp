#include "StartPlayingView.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>

#include "AudioManager.h"
#include "Camera.h"
#include "ModelComponent.h"
#include "MyMath.h"
#include "RenderQueue.h"

namespace
{
	Matrix4x4 MakeCameraFacingWorldMatrix(
		const Transform& transform,
		const Matrix4x4& cameraWorld)
	{
		// SRTのTranslateは画面上の配置としてカメラローカル座標のまま扱う。
		const Vector3 worldPosition =
			GameEngine::Math::Transforms(transform.translate, cameraWorld);

		// カメラの回転だけをビルボードへ適用し、位置は上で求めた座標へ設定する。
		Matrix4x4 billboardRotation = cameraWorld;
		billboardRotation.m[3][0] = 0.0f;
		billboardRotation.m[3][1] = 0.0f;
		billboardRotation.m[3][2] = 0.0f;

		return GameEngine::Math::MakeAffineMatrix(
			transform.scale,
			transform.rotate,
			{}) *
			billboardRotation *
			GameEngine::Math::MakeTranslateMatrix(worldPosition);
	}
}

StartPlayingView::StartPlayingView(
	const Models& models,
	const GameEngine::Camera* camera)
	: camera_(camera)
{
	constexpr Vector3 kCenterPosition = { 0.0f, 0.0f, 10.0f };
	constexpr Vector3 kRotation = { 1.57079637f, 3.14159274f, 0.0f };
	constexpr float kScale = 0.6f;
	constexpr float kSpacing = 0.55f;
	const float centerIndex = static_cast<float>(kModelCount - 1) * 0.5f;

	assert(camera_ && "StartPlayingView requires a camera.");
	for (std::size_t i = 0; i < models_.size(); ++i)
	{
		assert(models[i] && "playStartLogo0.obj through playStartLogo12.obj must be loaded.");
		if (!models[i])
		{
			continue;
		}

		models_[i] = std::make_unique<GameEngine::ModelComponent>(models[i]);
		models_[i]->SetEnableLighting(false);

		Transform& transform = models_[i]->worldTransform_.transform_;
		transform.scale = { kScale, kScale, kScale };
		transform.rotate = kRotation;
		transform.translate = kCenterPosition + Vector3{
			(static_cast<float>(i) - centerIndex) * kSpacing,
			0.0f,
			0.0f };

		const std::string groupName = "PlayStartLogo" + std::to_string(i);
		debugParameter_.Register("Scale", transform.scale, 0, groupName);
		debugParameter_.Register("Rotate", transform.rotate, 1, groupName);
		debugParameter_.Register("Translate", transform.translate, 2, groupName);
	}

	debugParameter_.Register("Duration", shakeDuration_, 0, "Shake");
	debugParameter_.Register("Amplitude", shakeAmplitude_, 1, "Shake");
	debugParameter_.Register("Frequency", shakeFrequency_, 2, "Shake");
	debugParameter_.Register("UpwardVelocity", burstUpwardVelocity_, 0, "BurstFall");
	debugParameter_.Register("HorizontalVelocity", burstHorizontalVelocity_, 1, "BurstFall");
	debugParameter_.Register("VelocityVariation", burstVelocityVariation_, 2, "BurstFall");
	debugParameter_.Register("Gravity", burstGravity_, 3, "BurstFall");
	debugParameter_.Register("RotationSpeed", burstRotationSpeed_, 4, "BurstFall");
	debugParameter_.Apply();
}

StartPlayingView::~StartPlayingView() = default;

void StartPlayingView::Start()
{
	ResetAnimation();
	isVisible_ = true;
	if ((std::max)(shakeDuration_, 0.0f) > 0.0f)
	{
		isShaking_ = true;
		auto& audioManager = GameEngine::AudioManager::GetInstance();
		const uint32_t shakeSoundHandle =
			audioManager.GetHandleByName("StartPlayingShake.mp3");
		audioManager.Play(shakeSoundHandle, 1.0f, false);
	}
	else
	{
		StartBurstFall();
	}
}

void StartPlayingView::Update(float deltaTime)
{
	debugParameter_.ApplyIfDirty();
	if (!isVisible_)
	{
		return;
	}

	const float elapsedStep = (std::max)(deltaTime, 0.0f);
	if (isShaking_)
	{
		const float duration = (std::max)(shakeDuration_, 0.0f);
		if (duration <= 0.0f)
		{
			ResetShake();
			StartBurstFall();
			UpdateBurstFall(elapsedStep);
			return;
		}

		const float elapsedBeforeUpdate = shakeElapsedTime_;
		shakeElapsedTime_ = (std::min)(
			shakeElapsedTime_ + elapsedStep,
			duration);
		const float progress = shakeElapsedTime_ / duration;
		const float strength =
			(std::max)(shakeAmplitude_, 0.0f) * (1.0f - progress);
		const float frequency = (std::max)(shakeFrequency_, 0.0f);

		for (std::size_t i = 0; i < shakeOffsets_.size(); ++i)
		{
			const float phase = static_cast<float>(i) * 1.37f;
			shakeOffsets_[i] = {
				std::sin(shakeElapsedTime_ * frequency + phase) * strength,
				std::cos(shakeElapsedTime_ * frequency * 0.83f + phase * 1.19f) * strength,
				0.0f };
		}

		if (shakeElapsedTime_ >= duration)
		{
			const float remainingTime = (std::max)(
				elapsedBeforeUpdate + elapsedStep - duration,
				0.0f);
			ResetShake();
			StartBurstFall();
			UpdateBurstFall(remainingTime);
		}
		return;
	}

	if (isBurstFalling_)
	{
		UpdateBurstFall(elapsedStep);
	}
}

void StartPlayingView::Stop()
{
	isVisible_ = false;
	ResetAnimation();
}

void StartPlayingView::Reset()
{
	isVisible_ = false;
	ResetAnimation();
}

void StartPlayingView::StartBurstFall()
{
	isBurstFalling_ = true;
	burstElapsedTime_ = 0.0f;
	burstOffsets_.fill({});
	burstRotationOffsets_.fill({});

	auto& audioManager = GameEngine::AudioManager::GetInstance();
	const uint32_t breakSoundHandle =
		audioManager.GetHandleByName("StartPlayingBreak.mp3");
	audioManager.Play(breakSoundHandle, 1.0f, false);

	const float centerIndex = static_cast<float>(kModelCount - 1) * 0.5f;
	const float horizontalVelocity = (std::max)(burstHorizontalVelocity_, 0.0f);
	const float upwardVelocity = (std::max)(burstUpwardVelocity_, 0.0f);
	const float velocityVariation = (std::max)(burstVelocityVariation_, 0.0f);
	const float rotationSpeed = (std::max)(burstRotationSpeed_, 0.0f);
	for (std::size_t i = 0; i < burstInitialVelocities_.size(); ++i)
	{
		const float index = static_cast<float>(i);
		const float phase = (index + 1.0f) * 2.13f;
		const float outwardDirection = centerIndex > 0.0f
			? (index - centerIndex) / centerIndex
			: 0.0f;
		burstInitialVelocities_[i] = {
			outwardDirection * horizontalVelocity +
				std::sin(phase) * velocityVariation,
			(std::max)(upwardVelocity +
				std::cos(phase) * velocityVariation, 0.0f),
			0.0f };

		const float rotationDirection = i % 2 == 0 ? 1.0f : -1.0f;
		burstAngularVelocities_[i] = rotationDirection * rotationSpeed *
			(0.75f + std::abs(std::sin(phase)) * 0.5f);
	}
}

void StartPlayingView::UpdateBurstFall(float deltaTime)
{
	burstElapsedTime_ += (std::max)(deltaTime, 0.0f);
	const float gravity = (std::max)(burstGravity_, 0.0f);
	const float gravityOffset = 0.5f * gravity * burstElapsedTime_ * burstElapsedTime_;
	for (std::size_t i = 0; i < burstOffsets_.size(); ++i)
	{
		burstOffsets_[i] = burstInitialVelocities_[i] * burstElapsedTime_;
		burstOffsets_[i].y -= gravityOffset;
		burstRotationOffsets_[i] = {
			0.0f,
			0.0f,
			burstAngularVelocities_[i] * burstElapsedTime_ };
	}
}

void StartPlayingView::ResetShake()
{
	isShaking_ = false;
	shakeElapsedTime_ = 0.0f;
	shakeOffsets_.fill({});
}

void StartPlayingView::ResetAnimation()
{
	ResetShake();
	isBurstFalling_ = false;
	burstElapsedTime_ = 0.0f;
	burstOffsets_.fill({});
	burstRotationOffsets_.fill({});
	burstInitialVelocities_.fill({});
	burstAngularVelocities_.fill(0.0f);
}

void StartPlayingView::Draw(GameEngine::RenderQueue* renderQueue)
{
	if (!isVisible_ || !camera_ || !renderQueue)
	{
		return;
	}

	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();

	for (std::size_t i = 0; i < models_.size(); ++i)
	{
		if (!models_[i])
		{
			continue;
		}

		const Transform& transform =
			models_[i]->worldTransform_.transform_;
		Transform displayTransform = transform;
		displayTransform.translate += shakeOffsets_[i] + burstOffsets_[i];
		displayTransform.rotate += burstRotationOffsets_[i];
		models_[i]->worldTransform_.UpdateWorldMatrix(
			MakeCameraFacingWorldMatrix(displayTransform, cameraWorld));
		models_[i]->Draw(renderQueue);
	}
}
