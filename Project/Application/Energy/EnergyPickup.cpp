#include "EnergyPickup.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Model.h"
#include "MyMath.h"
#include "RenderQueue.h"

using namespace GameEngine;

EnergyPickup::EnergyPickup(Model* model, GameEngine::Model* planeModel, GameEngine::TextureManager* textureManager) : particle_("EnergyEffect",16, textureManager, planeModel)
{
	assert(model != nullptr && "energy requires energy.obj");
	modelComponent_ = std::make_unique<ModelComponent>(model);
	//modelComponent_->materialData_->enableLighting = true;

	modelComponent_->SetHitGroup(6);
	modelComponent_->SetBufferMaterial(0, material_.GetMaterialSrvIndex());
}

void EnergyPickup::Spawn(
	EnergySize size,
	const Vector3& groundPosition,
	float fallHeight,
	float fallSpeed,
	const EnergyTypeSettings& typeSettings)
{
	// 着地点を保持したまま開始位置だけ上へずらし、Falling状態から始める。
	size_ = size;
	state_ = EnergyState::Falling;
	typeSettings_ = typeSettings;
	groundY_ = groundPosition.y;
	fallSpeed_ = (std::max)(fallSpeed, 0.0f);
	position_ = groundPosition;
	position_.y += (std::max)(fallHeight, 0.0f);
	animationTime_ = 0.0f;
	rotationY_ = 0.0f;
	floatingAmplitude_ = 0.0f;
	isHighlighted_ = false;
	SyncModel();
}

void EnergyPickup::SpawnOnGround(
	EnergySize size,
	const Vector3& groundPosition,
	const EnergyTypeSettings& typeSettings)
{
	// 敵ドロップは落下を経由せず、すぐロックオン可能なOnGroundにする。
	size_ = size;
	state_ = EnergyState::OnGround;
	typeSettings_ = typeSettings;
	groundY_ = groundPosition.y;
	fallSpeed_ = 0.0f;
	position_ = groundPosition;
	animationTime_ = 0.0f;
	rotationY_ = 0.0f;
	floatingAmplitude_ = 0.0f;
	isHighlighted_ = false;
	SyncModel();
}

void EnergyPickup::Reset()
{
	state_ = EnergyState::Inactive;
	position_ = {};
	groundY_ = 0.0f;
	fallSpeed_ = 0.0f;
	animationTime_ = 0.0f;
	rotationY_ = 0.0f;
	floatingAmplitude_ = 0.0f;
	isHighlighted_ = false;
}

void EnergyPickup::Update(
	float deltaTime,
	float floatingAmplitude,
	float floatingSpeed,
	float rotationSpeed)
{
	if (!IsActive()) {
		return;
	}

	// パーティクルの更新
	particle_.Update();

	const float safeDeltaTime = (std::max)(deltaTime, 0.0f);
	if (state_ == EnergyState::Falling) 
	{
		// 地面を通り抜けないよう、到達したフレームで位置をgroundY_へ固定する。
		position_.y -= fallSpeed_ * safeDeltaTime;
		if (position_.y <= groundY_) {
			position_.y = groundY_;
			state_ = EnergyState::OnGround;
			animationTime_ = 0.0f;
		}
	}

	if (state_ == EnergyState::OnGround || state_ == EnergyState::Reserved)
	{
		// 論理座標は地面に固定し、描画だけを上下させて距離判定へ影響させない。
		floatingAmplitude_ = (std::max)(floatingAmplitude, 0.0f);
		animationTime_ += safeDeltaTime * (std::max)(floatingSpeed, 0.0f);
		rotationY_ += safeDeltaTime * rotationSpeed;
		animationTime_ = std::fmod(animationTime_, TWO_PI);
		rotationY_ = std::fmod(rotationY_, TWO_PI);
	}
	else
	{
		floatingAmplitude_ = 0.0f;
	}

	SyncModel();
}

void EnergyPickup::Draw(RenderQueue* renderQueue)
{
	if (IsActive()) {
		//modelComponent_->DrawRaytracing(renderQueue);
		modelComponent_->DrawCustomRaytracing(renderQueue);

		// パーティクルの描画
		particle_.Draw();
	}
}

bool EnergyPickup::TryReserve()
{
	if (!IsTargetable()) 
	{
		return false;
	}

	// 予約後は別ユニットの検索対象から外れる。
	state_ = EnergyState::Reserved;
	return true;
}

bool EnergyPickup::BeginCarry() 
{
	if (!IsReserved())
	{
		return false;
	}

	state_ = EnergyState::Carried;
	return true;
}

void EnergyPickup::SetCarriedPosition(const Vector3& position) 
{
	if (!IsCarried()) 
	{
		return;
	}

	// ユニットが倒れた位置のXZを使い、Yだけはフィールドの地面へ戻す。
	position_ = position;
	SyncModel();
}

void EnergyPickup::DropOnGround(const Vector3& position)
{
	if (!IsReserved() && !IsCarried()) 
	{
		return;
	}

	position_ = position;
	position_.y = groundY_;
	state_ = EnergyState::OnGround;
	animationTime_ = 0.0f;
	floatingAmplitude_ = 0.0f;
	SyncModel();
}

int32_t EnergyPickup::Deliver()
{
	if (!IsCarried()) 
	{
		return 0;
	}

	// Resetで状態を破棄する前に獲得量を退避する。
	const int32_t deliveredValue = typeSettings_.value;
	Reset();
	return deliveredValue;
}

void EnergyPickup::ApplyTypeSettings(const EnergyTypeSettings& settings) 
{
	typeSettings_ = settings;
	typeSettings_.scale = (std::max)(typeSettings_.scale, 0.0f);
	typeSettings_.value = (std::max)(typeSettings_.value, 0);
	if (IsActive()) {
		SyncModel();
	}
}

void EnergyPickup::SetHighlighted(bool highlighted)
{
	if (isHighlighted_ == highlighted)
	{
		return;
	}

	isHighlighted_ = highlighted;
	if (IsActive()) 
	{
		SyncModel();
	}
}

void EnergyPickup::SyncModel()
{
	const float scale = typeSettings_.scale;
	modelComponent_->worldTransform_.transform_.scale = { scale, scale, scale };
	Vector3 displayPosition = position_;
	if (state_ == EnergyState::OnGround || state_ == EnergyState::Reserved)
	{
		displayPosition.y += std::sin(animationTime_) * floatingAmplitude_;
	}
	modelComponent_->worldTransform_.transform_.translate = displayPosition;
	modelComponent_->worldTransform_.transform_.rotate.y = rotationY_;
	Vector4 color = typeSettings_.color;
	if (isHighlighted_) 
	{
		// 選択中は元の色味を残しながら白へ寄せ、対象を判別しやすくする。
		color.x = color.x + (1.0f - color.x) * 0.65f;
		color.y = color.y + (1.0f - color.y) * 0.65f;
		color.z = color.z + (1.0f - color.z) * 0.65f;
	}
	modelComponent_->materialData_->color = color;

	material_.materialData_->baseColor = color;
	material_.materialData_->rimColor = typeSettings_.rimColor;
	material_.materialData_->dissolveEdgeColor = typeSettings_.dissolveEdgeColor;

	// 色を設定
	particle_.SetColor(color);
	// サイズを設定
	float eScale = scale * 1.5f;
	particle_.SetScale({ eScale, eScale, eScale });

	// 出現位置を設定
	Vector3 emitPos = modelComponent_->worldTransform_.transform_.translate;
	emitPos.y = groundY_;
	particle_.SetEmitterPos(emitPos);

	modelComponent_->Update();
}

