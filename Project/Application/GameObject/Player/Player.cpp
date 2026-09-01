#include "Application/GameObject/Player/Player.h"

#include "FPSCounter.h"
#include "InputCommand.h"
#include "Model.h"

using namespace GameEngine;

Player::Player(InputCommand* inputCommand, Model* model)
	: inputCommand_(inputCommand), model_(model) {
}

void Player::Initialize() {
	if (model_) {
		model_->SetDefaultIsEnableLight(true);
		model_->SetDefaultColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	worldTransform_.Initialize({
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }
	});
}

void Player::Update() {
	Move();

	worldTransform_.UpdateTransformMatrix();
}

void Player::Draw() {
	if (!model_) {
		return;
	}

	renderQueue_->SubmitModel(model_, worldTransform_);
}

void Player::Move() {
	if (!inputCommand_) {
		return;
	}

	Vector3 moveDirection = { 0.0f, 0.0f, 0.0f };

	if (inputCommand_->IsCommandActive("MoveUp")) {
		moveDirection.z += 1.0f;
	}
	if (inputCommand_->IsCommandActive("MoveDown")) {
		moveDirection.z -= 1.0f;
	}
	if (inputCommand_->IsCommandActive("MoveLeft")) {
		moveDirection.x -= 1.0f;
	}
	if (inputCommand_->IsCommandActive("MoveRight")) {
		moveDirection.x += 1.0f;
	}

	if (moveDirection.LengthSquared() > 0.0f) {
		moveDirection.Normalize();
		worldTransform_.transform_.translate += moveDirection * moveSpeed_ * FpsCounter::gameDeltaTime;
	}
}
