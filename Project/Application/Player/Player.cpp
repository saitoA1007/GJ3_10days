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
	trajectoryModel_(trajectoryModel), pikumiModel_(pikumiModel), field_(field)
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
	modelComponent_.materialData_->color = { 0.0f,0.0f,1.0f,1.0f };

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

	// 位置を設定
	field_->SetPos(modelComponent_.worldTransform_.transform_.translate);
  
	isMoving_ = false;

	for (auto& pikumi : pikumis_)
	{
		pikumi->SetFollowSpeed(pikumiFollowSpeed_);
		pikumi->SetDampening(pikumiDampening_);
		pikumi->SetScale(pikumiScale_);
		pikumi->SetFieldRadius(field_->GetFieldRadius());
		pikumi->SetAnimationParams(pikumiJumpHeight_, pikumiJumpFrequency_, pikumiSquashAmount_);
	}

	if (currentState_) 
	{
		currentState_->Update(this);
	}

	UpdateMoveAnimation();

	ClampToField();

	UpdateChargeThrow();

	// Pikumiの更新、回収
	UpdatePikumiFormations();

	// Pikumi同士の押し出し処理
	const float minSepDistance = pikumiSeparation_;
	for (size_t i = 0; i < pikumis_.size(); ++i)
	{
		if (pikumis_[i]->GetState() != PikumiState::kFollow) continue;

		for (size_t j = i + 1; j < pikumis_.size(); ++j)
		{
			if (pikumis_[j]->GetState() != PikumiState::kFollow) continue;

			// チャージ対象同士のPikumiは押し出し無効化
			if (isCharging_)
			{
				bool isACharged = (i < static_cast<size_t>(chargedPikumiCount_));
				bool isBCharged = (j < static_cast<size_t>(chargedPikumiCount_));
				if (isACharged && isBCharged) continue;
			}

			Vector3 posA = pikumis_[i]->GetWorldTransform().transform_.translate;
			Vector3 posB = pikumis_[j]->GetWorldTransform().transform_.translate;

			Vector3 diff = posA - posB;
			diff.y = 0.0f; 
			float dist = diff.Length();

			if (dist < minSepDistance && dist > 0.0001f)
			{
				Vector3 pushDir = diff / dist;
				float overlap = (minSepDistance - dist) * 0.5f;

				pikumis_[i]->SetPosition(posA + pushDir * overlap);
				pikumis_[j]->SetPosition(posB - pushDir * overlap);
			}
		}
	}

	for (auto& pikumi : pikumis_) {
		pikumi->Update();
	}
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
	Vector3 pos = modelComponent_.worldTransform_.transform_.translate;
	float distXZ = std::sqrt(pos.x * pos.x + pos.z * pos.z);

	float maxRadius = field_->GetFieldRadius() - colliderRadius_;

	if (distXZ > maxRadius && distXZ > 0.0f)
	{
		modelComponent_.worldTransform_.transform_.translate.x = (pos.x / distXZ) * maxRadius;
		modelComponent_.worldTransform_.transform_.translate.z = (pos.z / distXZ) * maxRadius;
	}
}

void Player::UpdateChargeThrow()
{
	// 追従中の Pikumi
	std::vector<Pikumi*> followPikumis;
	for (auto& pikumi : pikumis_)
	{
		if (pikumi->GetState() == PikumiState::kFollow)
		{
			followPikumis.push_back(pikumi.get());
		}
	}

	if (followPikumis.empty())
	{
		isCharging_ = false;
		chargeTimer_ = 0.0f;
		chargedPikumiCount_ = 0;
		ClearAllPikumiHighlights();
		return;
	}

	// Shot ボタン長押し
	if (inputCommand_->IsCommandActive("Shot"))
	{
		isCharging_ = true;
		chargeTimer_ += FpsCounter::deltaTime;
		if (chargeTimer_ > maxChargeTime_)
		{
			chargeTimer_ = maxChargeTime_;
		}

		// チャージ割合から投擲可能数
		float ratio = chargeTimer_ / maxChargeTime_;
		int totalFollowers = static_cast<int>(followPikumis.size());
		int throwableCount = 1 + static_cast<int>(ratio * (totalFollowers - 1));
		throwableCount = Math::Min(throwableCount, totalFollowers);
		chargedPikumiCount_ = throwableCount;

		// 有効範囲内の Pikumi をハイライト
		for (int i = 0; i < totalFollowers; ++i)
		{
			followPikumis[i]->SetHighlight(i < throwableCount);
		}
	}
	// Shot ボタンを離した瞬間
	else if (isCharging_)
	{
		float ratio = chargeTimer_ / maxChargeTime_;
		int totalFollowers = static_cast<int>(followPikumis.size());
		int throwableCount = 1 + static_cast<int>(ratio * (totalFollowers - 1));
		throwableCount = Math::Min(throwableCount, totalFollowers);

		Vector3 forward = Math::YawToDirection(currentYaw_);
		static std::mt19937 gen(std::random_device{}());
		std::uniform_real_distribution<float> spreadDist(-0.15f, 0.15f);

		// ハイライトされていた数の Pikumi だけを投擲
		lastThrownCount_ = throwableCount;
		++throwEventId_;
		for (int i = 0; i < throwableCount; ++i)
		{
			Vector3 spreadDir = forward + Vector3(spreadDist(gen), 0.0f, spreadDist(gen));
			followPikumis[i]->SetHighlight(false);
			followPikumis[i]->Throw(spreadDir, pikumiThrowSpeed_);
		}

		// チャージ状態のクリア
		isCharging_ = false;
		chargeTimer_ = 0.0f;
		chargedPikumiCount_ = 0;
		ClearAllPikumiHighlights();
	}
	else
	{
		chargedPikumiCount_ = 0;
	}
}

void Player::ClearAllPikumiHighlights()
{
	for (auto& pikumi : pikumis_)
	{
		pikumi->SetHighlight(false);
	}
}

void Player::UpdatePikumiFormations()
{
	if (pikumis_.empty()) return;

	Vector3 forward = Math::YawToDirection(currentYaw_);
	Vector3 right = Vector3(forward.z, 0.0f, -forward.x);

	static float time = 0.0f;
	time += FpsCounter::deltaTime;

	// 追従中の Pikumi リストを抽出
	std::vector<Pikumi*> followPikumis;
	for (auto& pikumi : pikumis_)
	{
		if (pikumi->GetState() == PikumiState::kFollow)
		{
			followPikumis.push_back(pikumi.get());
		}
	}

	if (followPikumis.empty()) return;

	// ブラックホール中心位置
	Vector3 vortexCenter = modelComponent_.worldTransform_.transform_.translate
		+ (right * chargeOffset_.x)
		+ (Vector3(0.0f, 1.0f, 0.0f) * chargeOffset_.y)
		+ (forward * chargeOffset_.z);

	// 通常追従の基準点
	Vector3 normalCenterPos = modelComponent_.worldTransform_.transform_.translate - forward * pikumiFollowOffset_;

	const float kGoldenAngle = 2.39996f;
	int unchargedIndex = 0;

	for (size_t i = 0; i < followPikumis.size(); ++i)
	{
		// チャージ中で、かつ溜まっている Pikumi
		if (isCharging_ && static_cast<int>(i) < chargedPikumiCount_)
		{
			// 均等に周回
			float angleProgress = (static_cast<float>(i) / Math::Max(1, chargedPikumiCount_)) * 2.0f * PI;
			float angle = time * chargeVortexSpeed_ + angleProgress;

			// 中心に向かって吸い込まれる
			float radiusRatio = 0.4f + 0.6f * (static_cast<float>(i) / Math::Max(1, chargedPikumiCount_));
			float currentRadius = chargeVortexRadius_ * radiusRatio;

			// 上下のゆらめき
			float heightOffset = std::sin(angle * 2.0f + followPikumis[i]->GetSeed()) * chargeVortexHeight_;

			Vector3 targetPos = vortexCenter + Vector3(
				std::cos(angle) * currentRadius,
				heightOffset,
				std::sin(angle) * currentRadius
			);

			followPikumis[i]->SetTargetFollowPosition(targetPos);
		}
		// 非チャージ
		else
		{
			float angle = unchargedIndex * kGoldenAngle;
			float r = pikumiSpacing_ * std::sqrt(static_cast<float>(unchargedIndex + 1));

			float wiggle = std::sin(time * 2.0f + followPikumis[i]->GetSeed()) * 0.08f;
			angle += wiggle;

			float localX = std::cos(angle) * r;
			float localZ = std::sin(angle) * r;

			Vector3 targetPos = normalCenterPos + (right * localX) + (forward * localZ);

			// フィールドクランプ
			float targetDistXZ = std::sqrt(targetPos.x * targetPos.x + targetPos.z * targetPos.z);
			if (targetDistXZ > field_->GetFieldRadius() && targetDistXZ > 0.0f)
			{
				targetPos.x = (targetPos.x / targetDistXZ) * field_->GetFieldRadius();
				targetPos.z = (targetPos.z / targetDistXZ) * field_->GetFieldRadius();
			}

			followPikumis[i]->SetTargetFollowPosition(targetPos);
			unchargedIndex++;
		}
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

float Player::CalculateDistanceToFieldBoundary(const Vector3& startPos, const Vector3& direction) const
{
	float R = field_->GetFieldRadius();

	float x0 = startPos.x;
	float z0 = startPos.z;
	float dx = direction.x;
	float dz = direction.z;

	// 方向ベクトルの正規化
	float dirLengthXZ = std::sqrt(dx * dx + dz * dz);
	if (dirLengthXZ < 0.0001f) return 0.0f;
	dx /= dirLengthXZ;
	dz /= dirLengthXZ;

	// Ray、Circle 交差判定
	float b = x0 * dx + z0 * dz;
	float c = (x0 * x0 + z0 * z0) - (R * R);

	float discriminant = b * b - c;
	if (discriminant < 0.0f) return 0.0f; // 交点なし

	// 前方側の交差点までの距離
	float t = -b + std::sqrt(discriminant);
	return Math::Max(0.0f, t);
}

void Player::DrawTrajectory()
{
	if (!isCharging_) return;

	Vector3 playerPos = modelComponent_.worldTransform_.transform_.translate;
	Vector3 forward = Math::YawToDirection(currentYaw_);
	float maxDistance = CalculateDistanceToFieldBoundary(playerPos, forward);

	if (trajectorySpacing_ <= 0.05f) return;

	// 端までに必要な球の個数を算出
	int sphereCount = static_cast<int>(maxDistance / trajectorySpacing_);
	if (sphereCount <= 0) return;

	// 不足分のコンポーネントを動的に生成
	while (static_cast<int>(trajectoryModelComponents_.size()) < sphereCount)
	{
		// Model* ポインタを渡してインスタンス化
		auto comp = std::make_unique<GameEngine::ModelComponent>(trajectoryModel_);
		comp->worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });
		trajectoryModelComponents_.push_back(std::move(comp));
	}

	// プレイヤーからフィールド端まで等間隔に配置して描画
	for (int i = 0; i < sphereCount; ++i)
	{
		float dist = (i + 1) * trajectorySpacing_;
		Vector3 pointPos = playerPos + forward * dist;
		pointPos.y = playerPos.y + trajectoryPosY_;

		auto& comp = trajectoryModelComponents_[i];
		comp->worldTransform_.transform_.translate = pointPos;
		comp->worldTransform_.transform_.scale = trajectoryScale_;
		comp->Update();

		comp->DrawRaytracing(renderQueue_);
	}
}

void Player::Draw()
{
	modelComponent_.DrawRaytracing(renderQueue_);
	rightHandModelComponent_.DrawRaytracing(renderQueue_);
	DrawTrajectory();

	for (auto& pikumi : pikumis_) 
	{
		pikumi->Draw();
	}
}

void Player::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) 
{
	OnCollisionStay(result);
}

void Player::OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result)
{
	if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kTower) ||
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
