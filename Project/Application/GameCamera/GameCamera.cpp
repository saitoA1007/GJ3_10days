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
	debugParameter_->Apply();
}

GameCamera::~GameCamera() = default;

void GameCamera::Initialize()
{
	Vector3 playerPosition{};
	if (player_)
	{
		playerPosition = player_->GetWorldTransform().GetWorldPosition();
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

	const Vector3 playerPosition = player_->GetWorldTransform().GetWorldPosition();
	const Vector3 targetPosition = CalculateCameraPosition(playerPosition);
	const float deltaTime = (std::max)(FpsCounter::deltaTime, 0.0f);
	const float interpolation = 1.0f - std::exp(-(std::max)(followSpeed_, 0.0f) * deltaTime);

	camera_.transform_.translate +=
		(targetPosition - camera_.transform_.translate) * interpolation;
	LookAtPlayer(playerPosition);
	camera_.Update();
}

Vector3 GameCamera::CalculateCameraPosition(const Vector3& playerPosition) const
{
	return playerPosition + Vector3{ 0.0f, height_, -distance_ };
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
