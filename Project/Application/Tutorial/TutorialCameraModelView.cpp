#include "TutorialCameraModelView.h"

#include <algorithm>
#include <cassert>

#include "Camera.h"
#include "ModelComponent.h"
#include "MyMath.h"
#include "RenderQueue.h"

TutorialCameraModelView::TutorialCameraModelView(
	GameEngine::Model* model,
	const GameEngine::Camera* camera,
	const std::string& parameterGroupName,
	const Settings& defaults)
	: camera_(camera), settings_(defaults), debugParameter_(parameterGroupName)
{
	assert(model && "tutorial model must be loaded.");
	assert(camera_ && "tutorial model requires a camera.");
	if (model)
	{
		model_ = std::make_unique<GameEngine::ModelComponent>(model);
		model_->SetEnableLighting(false);
		model_->SetColor({ 1.0f, 1.0f, 1.0f });
	}

	debugParameter_.Register("StartPosition", settings_.startPosition, 0);
	debugParameter_.Register("EndPosition", settings_.endPosition, 1);
	debugParameter_.Register("StartDelay", settings_.startDelay, 2);
	debugParameter_.Register("MoveDuration", settings_.moveDuration, 3);
	debugParameter_.Register("HoldDuration", settings_.holdDuration, 4);
	debugParameter_.Register("EaseType", settings_.easeType, 5);
	debugParameter_.Register("Rotation", settings_.rotation, 6);
	debugParameter_.Register("Scale", settings_.scale, 7);
	debugParameter_.Apply();
	Reset();
}

TutorialCameraModelView::~TutorialCameraModelView() = default;

void TutorialCameraModelView::Reset()
{
	currentPosition_ = settings_.startPosition;
	animationElapsed_ = 0.0f;
	wasInTutorial_ = false;
	hasEnteredTutorial_ = false;
	displayOffset_ = {};
	ResetSuccessAnimation();
}

void TutorialCameraModelView::ResetSuccessAnimation()
{
	successAnimationState_ = SuccessAnimationState::Inactive;
	successAnimationElapsed_ = 0.0f;
	currentRotationX_ = settings_.rotation.x;
}

void TutorialCameraModelView::StartSuccessAnimation(
	float targetRotationX,
	float rotateDuration,
	float returnDuration)
{
	if (successAnimationState_ != SuccessAnimationState::Inactive)
	{
		return;
	}

	successAnimationState_ = SuccessAnimationState::Rotate;
	successAnimationElapsed_ = 0.0f;
	successStartPosition_ = currentPosition_;
	successStartRotationX_ = settings_.rotation.x;
	successTargetRotationX_ = targetRotationX;
	successRotateDuration_ = (std::max)(rotateDuration, 0.0f);
	successReturnDuration_ = (std::max)(returnDuration, 0.0f);
	successEaseType_ = settings_.easeType;
	currentRotationX_ = successStartRotationX_;
}

void TutorialCameraModelView::StartReturnAnimation(float returnDuration)
{
	if (successAnimationState_ != SuccessAnimationState::Inactive)
	{
		return;
	}

	successAnimationState_ = SuccessAnimationState::ReturnDown;
	successAnimationElapsed_ = 0.0f;
	successStartPosition_ = currentPosition_;
	successReturnDuration_ = (std::max)(returnDuration, 0.0f);
	successEaseType_ = settings_.easeType;
	currentRotationX_ = settings_.rotation.x;
}

bool TutorialCameraModelView::IsSuccessAnimationComplete() const
{
	return successAnimationState_ == SuccessAnimationState::Complete;
}

bool TutorialCameraModelView::HasEntranceAnimationStarted() const
{
	const float startDelay = (std::max)(settings_.startDelay, 0.0f);
	return hasEnteredTutorial_ && animationElapsed_ > startDelay;
}

bool TutorialCameraModelView::IsEntranceAnimationComplete() const
{
	const float startDelay = (std::max)(settings_.startDelay, 0.0f);
	const float moveDuration = (std::max)(settings_.moveDuration, 0.0f);
	return animationElapsed_ >= startDelay + moveDuration;
}

float TutorialCameraModelView::GetHoldDuration() const
{
	return (std::max)(settings_.holdDuration, 0.0f);
}

float TutorialCameraModelView::GetMoveDuration() const
{
	return (std::max)(settings_.moveDuration, 0.0f);
}

void TutorialCameraModelView::SetColor(const Vector3& color)
{
	if (model_)
	{
		model_->SetColor(color);
	}
}

void TutorialCameraModelView::SetDisplayOffset(const Vector3& offset)
{
	displayOffset_ = offset;
}

void TutorialCameraModelView::Update(bool isTutorial, bool advanceAnimation, float deltaTime)
{
	debugParameter_.ApplyIfDirty();
	const bool isSuccessAnimationActive =
		successAnimationState_ != SuccessAnimationState::Inactive;

	if (isTutorial && !wasInTutorial_)
	{
		animationElapsed_ = 0.0f;
		hasEnteredTutorial_ = true;
	}
	else if (isTutorial && advanceAnimation && !isSuccessAnimationActive)
	{
		animationElapsed_ += (std::max)(deltaTime, 0.0f);
	}

	if (isTutorial && !isSuccessAnimationActive)
	{
		currentRotationX_ = settings_.rotation.x;
		const float startDelay = (std::max)(settings_.startDelay, 0.0f);
		const float moveDuration = (std::max)(settings_.moveDuration, 0.0f);
		if (animationElapsed_ <= startDelay)
		{
			currentPosition_ = settings_.startPosition;
		}
		else if (moveDuration <= 0.0f)
		{
			currentPosition_ = settings_.endPosition;
		}
		else
		{
			const float progress = (std::clamp)(
				(animationElapsed_ - startDelay) / moveDuration,
				0.0f,
				1.0f);
			currentPosition_ = GameEngine::Lerp(
				settings_.startPosition,
				settings_.endPosition,
				progress,
				settings_.easeType);
		}
	}
	else if (!hasEnteredTutorial_)
	{
		currentPosition_ = settings_.startPosition;
	}

	if (advanceAnimation && isSuccessAnimationActive)
	{
		UpdateSuccessAnimation(deltaTime);
	}

	wasInTutorial_ = isTutorial;
}

void TutorialCameraModelView::Draw(GameEngine::RenderQueue* renderQueue)
{
	if (!model_ || !camera_ || !renderQueue)
	{
		return;
	}

	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();
	const float scale = (std::max)(settings_.scale, 0.0f);
	Vector3 rotation = settings_.rotation;
	rotation.x = currentRotationX_;
	model_->worldTransform_.UpdateWorldMatrix(
		GameEngine::Math::MakeAffineMatrix(
			{ scale, scale, scale },
			rotation,
			currentPosition_ + displayOffset_) * cameraWorld);
	model_->Draw(renderQueue);
}

void TutorialCameraModelView::UpdateSuccessAnimation(float deltaTime)
{
	const float elapsedStep = (std::max)(deltaTime, 0.0f);
	switch (successAnimationState_)
	{
	case SuccessAnimationState::Rotate:
	{
		successAnimationElapsed_ = (std::min)(
			successAnimationElapsed_ + elapsedStep,
			successRotateDuration_);
		const float progress = successRotateDuration_ <= 0.0f
			? 1.0f
			: successAnimationElapsed_ / successRotateDuration_;
		currentRotationX_ = GameEngine::Lerp(
			successStartRotationX_,
			successTargetRotationX_,
			progress,
			successEaseType_);
		if (progress >= 1.0f)
		{
			successAnimationState_ = SuccessAnimationState::ReturnDown;
			successAnimationElapsed_ = 0.0f;
		}
		break;
	}
	case SuccessAnimationState::ReturnDown:
	{
		successAnimationElapsed_ = (std::min)(
			successAnimationElapsed_ + elapsedStep,
			successReturnDuration_);
		const float progress = successReturnDuration_ <= 0.0f
			? 1.0f
			: successAnimationElapsed_ / successReturnDuration_;
		currentPosition_ = GameEngine::Lerp(
			successStartPosition_,
			settings_.startPosition,
			progress,
			successEaseType_);
		if (progress >= 1.0f)
		{
			successAnimationState_ = SuccessAnimationState::Complete;
		}
		break;
	}
	case SuccessAnimationState::Inactive:
	case SuccessAnimationState::Complete:
		break;
	}
}
