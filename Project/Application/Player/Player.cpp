#include "Player.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "IPlayerState.h"
#include "PlayerStateIdle.h"

using namespace GameEngine;

Player::Player(InputCommand* inputCommand, Model* model)
	: inputCommand_(inputCommand), modelComponent_(model)
{
	// 初期化
	modelComponent_.worldTransform_.Initialize({ {3.0f,3.0f,3.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	debugParame_->Register("Scale", modelComponent_.worldTransform_.transform_.scale);
	debugParame_->Register("MoveSpeed", moveSpeed_, 0, "Move");
	debugParame_->Register("ColliderRadius", colliderRadius_, 0, "Collider");
	debugParame_->Register("ColliderOffsetPosY", colliderOffsetPosY_, 1, "Collider");

	// 当たり判定を設定
	collider_.SetRadius(colliderRadius_);
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(~kCollisionAttributePlayer);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kPlayer);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック登録
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
		});

	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionStay(result);
		});

	debugParame_->Apply();
}

void Player::Initialize() 
{
	modelComponent_.worldTransform_.transform_.translate = { 0.0f,0.0f,0.0f };
	// 初期状態は Idle
	ChangeState(std::make_unique<PlayerStateIdle>());
}

void Player::Update()
{
	debugParame_->ApplyIfDirty();

	if (currentState_) 
	{
		currentState_->Update(this);
	}

	modelComponent_.Update();

	collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition() + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetRadius(colliderRadius_);
}

void Player::Draw()
{
	modelComponent_.DrawRaytracing(renderQueue_);
}

void Player::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) 
{

}

void Player::OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result)
{

}

void Player::ChangeState(std::unique_ptr<IPlayerState> newState)
{
	currentState_ = std::move(newState);
	currentState_->Initialize(this);
}