#include "Player.h"

#include <random>
#include <algorithm>

#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "IPlayerState.h"
#include "PlayerStateIdle.h"
#include "Application/Field/Field.h"
#include "Application/Enemy/Enemy.h"
#include "EasingManager.h"

using namespace GameEngine;

Player::Player(InputCommand* inputCommand, Model* model, GameEngine::Model* pikumiModel,
	GameEngine::Model* rightHandModel, GameEngine::Model* trajectoryModel,
	Field* field, ImpactDetectionEffect* impactDetectionEffect)
	: inputCommand_(inputCommand), modelComponent_(model), rightHandModelComponent_(rightHandModel),
	trajectoryModel_(trajectoryModel), field_(field)
{
	// 初期化
	modelComponent_.worldTransform_.Initialize({ {3.0f,3.0f,3.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	rightHandModelComponent_.worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });
	rightHandModelComponent_.worldTransform_.SetParent(&modelComponent_.worldTransform_);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	debugParame_->Register("BaseScale", baseScale_, 0, "Transform");
	debugParame_->Register("MoveSpeed", moveSpeed_, 0, "Move");

	debugParame_->Register("SquashFrequency", squashFrequency_, 0, "Animation");
	debugParame_->Register("SquashAmount", squashStretchAmount_, 1, "Animation");

	debugParame_->Register("ColliderRadius", colliderRadius_, 0, "Collider");
	debugParame_->Register("ColliderOffsetPosY", colliderOffsetPosY_, 1, "Collider");

	debugParame_->Register("RightHandPos", rightHandOffsetPos_, 0, "RightHand");
	debugParame_->Register("RightHandScale", rightHandOffsetScale_, 1, "RightHand");
	debugParame_->Register("RightHandRot", rightHandOffsetRot_, 2, "RightHand");
	debugParame_->Register("ChargeTargetPos", rightHandChargeTargetPos_, 3, "RightHand");
	debugParame_->Register("ChargeTargetRot", rightHandChargeTargetRot_, 4, "RightHand");
	debugParame_->Register("ChargeMoveTime", rightHandChargeMoveTime_, 5, "RightHand");
	debugParame_->Register("ReturnTime", rightHandReturnTime_, 6, "RightHand");
	debugParame_->Register("ArcAmount", rightHandArcAmount_, 7, "RightHand");
	debugParame_->Register("ShakeAmount", rightHandShakeAmount_, 8, "RightHand");
	debugParame_->Register("ShakeFrequency", rightHandShakeFrequency_, 9, "RightHand");

	debugParame_->Register("Scale", pikumiScale_, 0, "Pikumi");
	debugParame_->Register("FollowOffset", pikumiFollowOffset_, 1, "Pikumi");
	debugParame_->Register("Spacing", pikumiSpacing_, 2, "Pikumi");         
	debugParame_->Register("Separation", pikumiSeparation_, 3, "Pikumi");   
	debugParame_->Register("FollowSpeed", pikumiFollowSpeed_, 4, "Pikumi");
	debugParame_->Register("ThrowSpeed", pikumiThrowSpeed_, 5, "Pikumi");
	debugParame_->Register("Dampening", pikumiDampening_, 6, "Pikumi");
	debugParame_->Register("CollectRadius", pikumiCollectRadius_, 7, "Pikumi");
	debugParame_->Register("MaxChargeTime", maxChargeTime_, 8, "Pikumi");
	debugParame_->Register("JumpHeight", pikumiJumpHeight_, 9, "Pikumi");
	debugParame_->Register("JumpFrequency", pikumiJumpFrequency_, 10, "Pikumi");
	debugParame_->Register("SquashAmount", pikumiSquashAmount_, 11, "Pikumi");

	debugParame_->Register("ChargeOffset", chargeOffset_, 0, "ChargeVortex");
	debugParame_->Register("VortexRadius", chargeVortexRadius_, 1, "ChargeVortex");
	debugParame_->Register("VortexSpeed", chargeVortexSpeed_, 2, "ChargeVortex");
	debugParame_->Register("VortexHeight", chargeVortexHeight_, 3, "ChargeVortex");

	debugParame_->Register("Spacing", trajectorySpacing_, 0, "Trajectory");
	debugParame_->Register("Scale", trajectoryScale_, 1, "Trajectory");
	debugParame_->Register("PosY", trajectoryPosY_, 2, "Trajectory");

	// 当たり判定を設定
	collider_.SetRadius(colliderRadius_);
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(kCollisionAttributePikumi | kCollisionAttributeTower | kCollisionAttributeEnemy);
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
	modelComponent_.materialData_->color = { 0.0f,0.0f,1.0f,1.0f };

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

	UpdateMoveAnimation();

	ClampToField();

	UpdateChargeThrow();

	// Pikumiの更新、回収
	UpdatePikumiFormations();


	CheckPikumiCollection();

	modelComponent_.Update();

	UpdateRightHandAnimation();

	collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition() + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetRadius(colliderRadius_);
}

void Player::UpdateMoveAnimation()
{
	if (isMoving_)
	{
		moveAnimTimer_ += FpsCounter::deltaTime * squashFrequency_;

		float bounce = std::abs(std::sin(moveAnimTimer_));
		float factor = (bounce - 0.5f) * 2.0f;

		Vector3 currentScale;
		currentScale.x = baseScale_.x * (1.0f - factor * squashStretchAmount_ * 0.5f);
		currentScale.y = baseScale_.y * (1.0f + factor * squashStretchAmount_);
		currentScale.z = baseScale_.z * (1.0f - factor * squashStretchAmount_ * 0.5f);

		modelComponent_.worldTransform_.transform_.scale = currentScale;
	}
	else
	{
		moveAnimTimer_ = 0.0f;
		modelComponent_.worldTransform_.transform_.scale = Lerp(
			modelComponent_.worldTransform_.transform_.scale,
			baseScale_,
			15.0f * FpsCounter::deltaTime
		);
	}
}

void Player::UpdateRightHandAnimation()
{
	static float handAnimTime = 0.0f;
	handAnimTime += FpsCounter::deltaTime;

	// チャージ状態に応じた移動進行度の更新
	if (isCharging_)
	{
		if (rightHandChargeMoveTime_ > 0.0f)
		{
			rightHandChargeProgress_ += FpsCounter::deltaTime / rightHandChargeMoveTime_;
		}
		else
		{
			rightHandChargeProgress_ = 1.0f;
		}
	}
	else
	{
		if (rightHandReturnTime_ > 0.0f)
		{
			rightHandChargeProgress_ -= FpsCounter::deltaTime / rightHandReturnTime_;
		}
		else
		{
			rightHandChargeProgress_ = 0.0f;
		}
	}

	// 進行度をクランプ
	rightHandChargeProgress_ = std::clamp(rightHandChargeProgress_, 0.0f, 1.0f);

	float t = rightHandChargeProgress_;
	float smoothT = t * t * (3.0f - 2.0f * t);

	// 目標位置への旋回移動
	Vector3 currentPos = Lerp(rightHandOffsetPos_, rightHandChargeTargetPos_, smoothT);

	// 旋回運動のカーブ加算
	float arcFactor = std::sin(smoothT * PI) * rightHandArcAmount_;
	currentPos.x += arcFactor;

	Vector3 currentRot = Lerp(rightHandOffsetRot_, rightHandChargeTargetRot_, smoothT);

	// チャージ中のシェイク
	if (isCharging_ && rightHandChargeProgress_ > 0.0f)
	{
		float shakeFactor = smoothT * rightHandShakeAmount_;

		float shakeX = std::sin(handAnimTime * rightHandShakeFrequency_) * shakeFactor;
		float shakeY = std::cos(handAnimTime * rightHandShakeFrequency_ * 1.2f) * shakeFactor;
		float shakeZ = std::sin(handAnimTime * rightHandShakeFrequency_ * 0.8f) * shakeFactor;

		currentPos += Vector3(shakeX, shakeY, shakeZ);
	}

	rightHandModelComponent_.worldTransform_.transform_.translate = currentPos;
	rightHandModelComponent_.worldTransform_.transform_.rotate = currentRot;
	rightHandModelComponent_.worldTransform_.transform_.scale = rightHandOffsetScale_;

	rightHandModelComponent_.Update();
}

void Player::ClampToField()
{
	
}

void Player::UpdateChargeThrow()
{

}

void Player::ClearAllPikumiHighlights()
{

}

void Player::UpdatePikumiFormations()
{
	
	Vector3 forward = Math::YawToDirection(currentYaw_);
	Vector3 right = Vector3(forward.z, 0.0f, -forward.x);

	static float time = 0.0f;
	time += FpsCounter::deltaTime;


}

void Player::ThrowAllPikumis()
{
	Vector3 forward = Math::YawToDirection(currentYaw_);

	static std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> spreadDist(-0.25f, 0.25f);

}

void Player::CheckPikumiCollection()
{
	
}

void Player::DrawTrajectory()
{
	
}

void Player::Draw()
{
	modelComponent_.DrawRaytracing(renderQueue_);
	rightHandModelComponent_.DrawRaytracing(renderQueue_);
	DrawTrajectory();

}

void Player::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) 
{
	OnCollisionStay(result);
}

void Player::OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result)
{
	if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kRocket) ||
		result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy))
	{
		Vector3 normal = result.contactNormal;
		normal.y = 0.0f;

		if (normal.LengthSquared() > 0.0001f)
		{
			normal.Normalize();
		}

		Vector3 playerPos = modelComponent_.worldTransform_.transform_.translate;
		Vector3 targetPos = { 0.0f, 0.0f, 0.0f }; 

		if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy) && result.userData.object)
		{
			auto enemy = result.userData.As<Enemy>();
			targetPos = enemy->GetPosition();
		}

		// 相手の中心からプレイヤーに向かう方向
		Vector3 dirFromTarget = Vector3(playerPos.x - targetPos.x, 0.0f, playerPos.z - targetPos.z);

		if (dirFromTarget.LengthSquared() > 0.0001f)
		{
			dirFromTarget.Normalize();
			// 法線が相手の内側を向いている場合、外側向きに反転
			if (Math::Dot(normal, dirFromTarget) < 0.0f)
			{
				normal = normal * -1.0f;
			}
		}

		// めり込んだ分だけ押し戻す
		modelComponent_.worldTransform_.transform_.translate += normal * result.penetrationDepth;

		modelComponent_.Update();
	}
}

void Player::ChangeState(std::unique_ptr<IPlayerState> newState)
{
	currentState_ = std::move(newState);
	currentState_->Initialize(this);
}
