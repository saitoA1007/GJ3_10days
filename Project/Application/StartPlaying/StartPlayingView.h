#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "DebugParameter.h"
#include "Vector3.h"

namespace GameEngine
{
	class Camera;
	class Model;
	class ModelComponent;
	class RenderQueue;
}

/// @brief StartPlaying中に表示するロゴモデルをまとめて管理する。
class StartPlayingView final
{
public:
	static constexpr std::size_t kModelCount = 13;
	using Models = std::array<GameEngine::Model*, kModelCount>;

	StartPlayingView(
		const Models& models,
		const GameEngine::Camera* camera);
	~StartPlayingView();

	void Start();
	void Update(float deltaTime);
	void Stop();
	void Reset();
	void Draw(GameEngine::RenderQueue* renderQueue);

private:
	void StartBurstFall();
	void UpdateBurstFall(float deltaTime);
	void ResetShake();
	void ResetAnimation();

	const GameEngine::Camera* camera_ = nullptr;
	std::array<std::unique_ptr<GameEngine::ModelComponent>, kModelCount> models_;
	std::array<Vector3, kModelCount> shakeOffsets_{};
	std::array<Vector3, kModelCount> burstOffsets_{};
	std::array<Vector3, kModelCount> burstRotationOffsets_{};
	std::array<Vector3, kModelCount> burstInitialVelocities_{};
	std::array<float, kModelCount> burstAngularVelocities_{};
	GameEngine::DebugParameter debugParameter_{ "StartPlayingView" };
	float shakeDuration_ = 0.5f;
	float shakeAmplitude_ = 0.12f;
	float shakeFrequency_ = 35.0f;
	float burstUpwardVelocity_ = 5.5f;
	float burstHorizontalVelocity_ = 1.5f;
	float burstVelocityVariation_ = 0.8f;
	float burstGravity_ = 9.8f;
	float burstRotationSpeed_ = 3.0f;
	float shakeElapsedTime_ = 0.0f;
	float burstElapsedTime_ = 0.0f;
	bool isShaking_ = false;
	bool isBurstFalling_ = false;
	bool isVisible_ = false;
};
