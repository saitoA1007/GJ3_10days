#include "Player.h"

#include <random>

#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "IPlayerState.h"
#include "PlayerStateIdle.h"
#include "Application/Field/Field.h"

using namespace GameEngine;

Player::Player(InputCommand* inputCommand, Model* model, GameEngine::Model* pikumiModel, Field* field, ImpactDetectionEffect* impactDetectionEffect)
	: inputCommand_(inputCommand), modelComponent_(model), pikumiModel_(pikumiModel), field_(field)
{
	// 初期化
	modelComponent_.worldTransform_.Initialize({ {3.0f,3.0f,3.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	debugParame_->Register("Scale", modelComponent_.worldTransform_.transform_.scale);
	debugParame_->Register("MoveSpeed", moveSpeed_, 0, "Move");
	debugParame_->Register("ColliderRadius", colliderRadius_, 0, "Collider");
	debugParame_->Register("ColliderOffsetPosY", colliderOffsetPosY_, 1, "Collider");

	debugParame_->Register("Scale", pikumiScale_, 0, "Pikumi");
	debugParame_->Register("FollowOffset", pikumiFollowOffset_, 1, "Pikumi");
	debugParame_->Register("Radius", pikumiRadius_, 2, "Pikumi");
	debugParame_->Register("FollowSpeed", pikumiFollowSpeed_, 3, "Pikumi");
	debugParame_->Register("ThrowSpeed", pikumiThrowSpeed_, 4, "Pikumi");
	debugParame_->Register("Dampening", pikumiDampening_, 5, "Pikumi");
	debugParame_->Register("CollectRadius", pikumiCollectRadius_, 6, "Pikumi");

	// 当たり判定を設定
	collider_.SetRadius(colliderRadius_);
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributePikumi);
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

	pikumis_.clear();
	for (int i = 0; i < pikumiCount_; ++i)
	{
		pikumis_.push_back(std::make_unique<Pikumi>(pikumiModel_, impactDetectionEffect));
	}

	debugParame_->Apply();
}

void Player::Initialize() 
{
	modelComponent_.worldTransform_.transform_.translate = { 0.0f,0.0f,0.0f };

	// Pikumiの初期化
	for (auto& pikumi : pikumis_)
	{
		pikumi->Initialize();
		pikumi->SetPosition(modelComponent_.worldTransform_.transform_.translate);
	}

	// 初期状態は Idle
	ChangeState(std::make_unique<PlayerStateIdle>());
}

void Player::Update()
{
	debugParame_->ApplyIfDirty();

	for (auto& pikumi : pikumis_)
	{
		pikumi->SetFollowSpeed(pikumiFollowSpeed_);
		pikumi->SetDampening(pikumiDampening_);
		pikumi->SetScale(pikumiScale_);
		pikumi->SetFieldRadius(field_->GetFieldRadius());
	}

	if (currentState_) 
	{
		currentState_->Update(this);
	}

	ClampToField();

	if (inputCommand_->IsCommandActive("Shot"))
	{
		ThrowAllPikumis();
	}

	// Pikumiの更新、回収
	UpdatePikumiFormations();
	for (auto& pikumi : pikumis_) {
		pikumi->Update();
	}
	CheckPikumiCollection();

	modelComponent_.Update();

	collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition() + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetRadius(colliderRadius_);
}

void Player::ClampToField()
{
	Vector3 pos = modelComponent_.worldTransform_.transform_.translate;
	float distXZ = std::sqrt(pos.x * pos.x + pos.z * pos.z);

	// プレイヤーの判定半径分だけ内側に制限
	float maxRadius = field_->GetFieldRadius() - colliderRadius_;

	if (distXZ > maxRadius && distXZ > 0.0f)
	{
		modelComponent_.worldTransform_.transform_.translate.x = (pos.x / distXZ) * maxRadius;
		modelComponent_.worldTransform_.transform_.translate.z = (pos.z / distXZ) * maxRadius;
	}
}

void Player::UpdatePikumiFormations()
{
	if (static_cast<int>(pikumis_.size()) == 0) return;

	Vector3 forward = Math::YawToDirection(currentYaw_);
	Vector3 right = Vector3(forward.z, 0.0f, -forward.x);

	// プレイヤーの後方を基準点
	Vector3 centerPos = modelComponent_.worldTransform_.transform_.translate - forward * pikumiFollowOffset_;

	static float time = 0.0f;
	time += FpsCounter::deltaTime;

	for (int i = 0; i < static_cast<int>(pikumis_.size()); ++i)
	{
		if (pikumis_[i]->GetState() != PikumiState::kFollow) continue;

		float wiggle = std::sin(time * 2.5f + pikumis_[i]->GetSeed()) * 0.12f;
		float angle = pikumis_[i]->GetAngleOffset() + wiggle;

		float r = pikumiRadius_ * pikumis_[i]->GetRadiusRatio();

		float localX = std::cos(angle) * r;
		float localZ = std::sin(angle) * r;

		// プレイヤーの向きに合わせてワールド座標に変換
		Vector3 targetPos = centerPos + (right * localX) + (forward * localZ);

		// フィールド内にクランプ
		float targetDistXZ = std::sqrt(targetPos.x * targetPos.x + targetPos.z * targetPos.z);
		if (targetDistXZ > field_->GetFieldRadius() && targetDistXZ > 0.0f)
		{
			targetPos.x = (targetPos.x / targetDistXZ) * field_->GetFieldRadius();
			targetPos.z = (targetPos.z / targetDistXZ) * field_->GetFieldRadius();
		}

		pikumis_[i]->SetTargetFollowPosition(targetPos);
	}
}

void Player::ThrowAllPikumis()
{
	Vector3 forward = Math::YawToDirection(currentYaw_);

	static std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> spreadDist(-0.25f, 0.25f);

	for (auto& pikumi : pikumis_)
	{
		if (pikumi->GetState() == PikumiState::kFollow)
		{
			Vector3 spreadDir = forward + Vector3(spreadDist(gen), 0.0f, spreadDist(gen));
			pikumi->Throw(spreadDir, pikumiThrowSpeed_);
		}
	}
}

void Player::CheckPikumiCollection()
{
	float collectRadiusSq = pikumiCollectRadius_ * pikumiCollectRadius_;

	for (auto& pikumi : pikumis_)
	{
		if (pikumi->GetState() == PikumiState::kIdle)
		{
			Vector3 diff = pikumi->GetWorldTransform().transform_.translate - modelComponent_.worldTransform_.transform_.translate;
			if (diff.LengthSquared() <= collectRadiusSq)
			{
				pikumi->Collect();
			}
		}
	}
}

void Player::Draw()
{
	modelComponent_.DrawRaytracing(renderQueue_);

	for (auto& pikumi : pikumis_) 
	{
		pikumi->Draw();
	}
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