#include "GameCamera.h"

#include <algorithm>
#include <cmath>

#include "Application/Player/Player.h"
#include "DebugParameter.h"
#include "FPSCounter.h"
#include "MyMath.h"

using namespace GameEngine;

GameCamera::GameCamera(Player* player)
	: player_(player)
{
	// Playerなどの通常オブジェクトが移動した後に追従する。
	SetUpdateOrder(100);

	debugParameter_ = std::make_unique<DebugParameter>("GameCamera");
	debugParameter_->Register("Height", height_, 0);
	debugParameter_->Register("Distance", distance_, 1);
	debugParameter_->Register("LookAtHeight", lookAtHeight_, 2);
	debugParameter_->Register("FollowSpeed", followSpeed_, 3);
	debugParameter_->Register("Rotate", rotateOffset_, 4);
	debugParameter_->Register("RotateWithMovement", rotateWithMovement_, 5);
	debugParameter_->Register("ThrowCountThreshold", throwCountThreshold_, 0, "ThrowHeightAnimation");
	debugParameter_->Register("RiseHeight", riseHeight_, 1, "ThrowHeightAnimation");
	debugParameter_->Register("RiseDuration", riseDuration_, 2, "ThrowHeightAnimation");
	debugParameter_->Register("HoldDuration", holdDuration_, 3, "ThrowHeightAnimation");
	debugParameter_->Register("ReturnDuration", returnDuration_, 4, "ThrowHeightAnimation");
	debugParameter_->Register("ReturnHeight", returnHeight_, 5, "ThrowHeightAnimation");
	debugParameter_->Register("MoveToOrigin", moveToOriginWithHeightAnimation_, 6, "ThrowHeightAnimation");
	debugParameter_->Register("MoveTargetPosition", moveTargetPosition_, 7, "ThrowHeightAnimation");
	debugParameter_->Apply();
}

GameCamera::~GameCamera() = default;

void GameCamera::Initialize()
{
	Vector3 playerPosition{};
	if (player_)
	{
		playerPosition = player_->GetWorldTransform().GetWorldPosition();
		lastHandledThrowEventId_ = player_->GetThrowEventId();
	}

	camera_.Initialize(
		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, CalculateCameraPosition(playerPosition) },
		1280,
		720);
	LookAtPlayer(playerPosition);
	camera_.Update();
	renderQueue_->SetCamera(&camera_);
}

void GameCamera::Update()
{
	debugParameter_->ApplyIfDirty();

	if (!player_)
	{
		return;
	}

	const uint32_t throwEventId = player_->GetThrowEventId();
	if (throwEventId != lastHandledThrowEventId_)
	{
		lastHandledThrowEventId_ = throwEventId;
		if (player_->GetLastThrownCount() >= (std::max)(throwCountThreshold_, 1))
		{
			StartThrowHeightAnimation();
		}
	}

	const Vector3 playerPosition = player_->GetWorldTransform().GetWorldPosition();
	const float deltaTime = (std::max)(FpsCounter::deltaTime, 0.0f);
	UpdateThrowHeightAnimation(deltaTime);

	const Vector3 animatedTargetPosition = CalculateCameraPosition(playerPosition);
	const float interpolation = 1.0f - std::exp(-(std::max)(followSpeed_, 0.0f) * deltaTime);

	camera_.transform_.translate +=
		(animatedTargetPosition - camera_.transform_.translate) * interpolation;
	LookAtPlayer(playerPosition);
	camera_.Update();
}

void GameCamera::StartThrowHeightAnimation()
{
	throwHeightAnimationState_ = ThrowHeightAnimationState::Rising;
	throwHeightAnimationTimer_.Start((std::max)(riseDuration_, 0.0f));
	throwHeightAnimationStart_ = height_;
	originMovementStart_ = originMovementProgress_;
}

void GameCamera::UpdateThrowHeightAnimation(float deltaTime)
{
	float remainingTime = (std::max)(deltaTime, 0.0f);

	// 秒数が0のフェーズも同一フレーム内で進める。
	for (int i = 0; i < 4; ++i)
	{
		switch (throwHeightAnimationState_)
		{
		case ThrowHeightAnimationState::Idle:
			originMovementProgress_ = 0.0f;
			return;

		case ThrowHeightAnimationState::Rising:
		{
			const float duration = (std::max)(riseDuration_, 0.0f);
			const float elapsedBeforeUpdate = throwHeightAnimationTimer_.GetElapsedTime();
			throwHeightAnimationTimer_.SetDuration(duration);
			throwHeightAnimationTimer_.Update(remainingTime);
			const float progress = throwHeightAnimationTimer_.GetProgress();
			height_ = throwHeightAnimationStart_ + (riseHeight_ - throwHeightAnimationStart_) * progress;
			originMovementProgress_ = moveToOriginWithHeightAnimation_
				? originMovementStart_ + (1.0f - originMovementStart_) * progress
				: 0.0f;
			if (!throwHeightAnimationTimer_.IsFinished())
			{
				return;
			}

			remainingTime = (std::max)(elapsedBeforeUpdate + remainingTime - duration, 0.0f);
			throwHeightAnimationState_ = ThrowHeightAnimationState::Holding;
			throwHeightAnimationTimer_.Start((std::max)(holdDuration_, 0.0f));
			continue;
		}

		case ThrowHeightAnimationState::Holding:
		{
			height_ = riseHeight_;
			originMovementProgress_ = moveToOriginWithHeightAnimation_ ? 1.0f : 0.0f;
			const float duration = (std::max)(holdDuration_, 0.0f);
			const float elapsedBeforeUpdate = throwHeightAnimationTimer_.GetElapsedTime();
			throwHeightAnimationTimer_.SetDuration(duration);
			throwHeightAnimationTimer_.Update(remainingTime);
			if (!throwHeightAnimationTimer_.IsFinished())
			{
				return;
			}

			remainingTime = (std::max)(elapsedBeforeUpdate + remainingTime - duration, 0.0f);
			throwHeightAnimationState_ = ThrowHeightAnimationState::Returning;
			throwHeightAnimationTimer_.Start((std::max)(returnDuration_, 0.0f));
			throwHeightAnimationStart_ = height_;
			originMovementStart_ = originMovementProgress_;
			continue;
		}

		case ThrowHeightAnimationState::Returning:
		{
			const float duration = (std::max)(returnDuration_, 0.0f);
			throwHeightAnimationTimer_.SetDuration(duration);
			throwHeightAnimationTimer_.Update(remainingTime);
			const float progress = throwHeightAnimationTimer_.GetProgress();
			height_ = throwHeightAnimationStart_ + (returnHeight_ - throwHeightAnimationStart_) * progress;
			originMovementProgress_ = moveToOriginWithHeightAnimation_
				? originMovementStart_ * (1.0f - progress)
				: 0.0f;
			if (!throwHeightAnimationTimer_.IsFinished())
			{
				return;
			}

			height_ = returnHeight_;
			originMovementProgress_ = 0.0f;
			throwHeightAnimationState_ = ThrowHeightAnimationState::Idle;
			throwHeightAnimationTimer_.Reset();
			return;
		}
		}
	}
}

Vector3 GameCamera::CalculateCameraPosition(const Vector3& playerPosition) const
{
	const float originProgress = moveToOriginWithHeightAnimation_ ? originMovementProgress_ : 0.0f;
	const Vector3 followPosition =
		playerPosition * (1.0f - originProgress) + moveTargetPosition_ * originProgress;
	return followPosition + Vector3{ 0.0f, height_, -distance_ };
}

void GameCamera::LookAtPlayer(const Vector3& playerPosition)
{
	Vector3 lookDirection;
	if (rotateWithMovement_)
	{
		const Vector3 lookAtPosition = playerPosition + Vector3{ 0.0f, lookAtHeight_, 0.0f };
		lookDirection = lookAtPosition - camera_.transform_.translate;
	}
	else
	{
		// 追従の補間による位置のずれに影響されない固定角度。
		lookDirection = { 0.0f, lookAtHeight_ - height_, distance_ };
	}

	camera_.transform_.rotate = Math::DirectionToEuler(lookDirection) + rotateOffset_;
}
