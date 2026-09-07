#pragma once

#include <memory>
#include <string>

#include "DebugParameter.h"
#include "EasingManager.h"
#include "Vector3.h"

namespace GameEngine
{
	class Camera;
	class Model;
	class ModelComponent;
	class RenderQueue;
}

/// @brief カメラローカル座標で表示するチュートリアル用モデル。
class TutorialCameraModelView final
{
public:
	struct Settings
	{
		Vector3 startPosition = { 0.0f, 2.8f, 10.0f };
		Vector3 endPosition = { 0.0f, 2.8f, 10.0f };
		Vector3 rotation = { 1.57079637f, 3.14159274f, 0.0f };
		float scale = 0.5f;
		float startDelay = 0.0f;
		float moveDuration = 1.0f;
		EaseType easeType = EaseType::kEaseOutCubic;
	};

	TutorialCameraModelView(
		GameEngine::Model* model,
		const GameEngine::Camera* camera,
		const std::string& parameterGroupName,
		const Settings& defaults = {});
	~TutorialCameraModelView();

	void Reset();
	void ResetSuccessAnimation();
	void StartSuccessAnimation(
		float targetRotationX,
		float rotateDuration,
		float returnDuration);
	bool IsSuccessAnimationComplete() const;

	void SetColor(const Vector3& color);
	void SetDisplayOffset(const Vector3& offset);
	void Update(bool isTutorial, bool advanceAnimation, float deltaTime);
	void Draw(GameEngine::RenderQueue* renderQueue);

private:
	enum class SuccessAnimationState
	{
		Inactive,
		Rotate,
		ReturnDown,
		Complete,
	};

	void UpdateSuccessAnimation(float deltaTime);

	const GameEngine::Camera* camera_ = nullptr;
	std::unique_ptr<GameEngine::ModelComponent> model_;
	Settings settings_;
	Vector3 currentPosition_ = {};
	Vector3 displayOffset_ = {};
	GameEngine::DebugParameter debugParameter_;
	float animationElapsed_ = 0.0f;
	SuccessAnimationState successAnimationState_ = SuccessAnimationState::Inactive;
	Vector3 successStartPosition_ = {};
	float currentRotationX_ = 0.0f;
	float successStartRotationX_ = 0.0f;
	float successTargetRotationX_ = 0.0f;
	float successRotateDuration_ = 0.0f;
	float successReturnDuration_ = 0.0f;
	float successAnimationElapsed_ = 0.0f;
	EaseType successEaseType_ = EaseType::kLinear;
	bool wasInTutorial_ = false;
	bool hasEnteredTutorial_ = false;
};
