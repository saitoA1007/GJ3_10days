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
	float appearDuration,
	const EnergyTypeSettings& typeSettings)
{
	// 着地点を保持したまま開始位置だけ上へずらし、Falling状態から始める。
	size_ = size;
	state_ = EnergyState::Appearing;
	typeSettings_ = typeSettings;
	groundY_ = groundPosition.y;
	position_ = groundPosition; 

	animationTime_ = 0.0f;
	rotationY_ = 0.0f;
	floatingAmplitude_ = 0.0f;
	isHighlighted_ = false;
	rainbowHue_ = 0.0f;
	lifetimeTimer_ = 0.0f;
	dissolveTimer_ = 0.0f;
	isDissolving_ = false;
	lifetimeEnabled_ = true;

	// ディゾルブの初期設定
	appearTime_ = 0.0f;
	appearDuration_ = appearDuration;
	material_.materialData_->dissolveThreshold = 1.0f;

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
	rainbowHue_ = 0.0f;
	lifetimeTimer_ = 0.0f;
	dissolveTimer_ = 0.0f;
	isDissolving_ = false;
	lifetimeEnabled_ = true;
	material_.materialData_->dissolveThreshold = 0.0f;
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
	rainbowHue_ = 0.0f;
	lifetimeTimer_ = 0.0f;
	dissolveTimer_ = 0.0f;
	isDissolving_ = false;
	lifetimeEnabled_ = true;
	material_.materialData_->dissolveThreshold = 0.0f;
}

void EnergyPickup::Update(
	float deltaTime,
	float floatingAmplitude,
	float floatingSpeed,
	float rotationSpeed, 
	float lifetime,
	float dissolveDuration)
{
	if (!IsActive()) {
		return;
	}

	// パーティクルの更新
	particle_.SetEmitterPos(modelComponent_->worldTransform_.transform_.translate);
	particle_.Update();

	const float safeDeltaTime = (std::max)(deltaTime, 0.0f);

	// 出現ディゾルブ処理
	if (state_ == EnergyState::Appearing)
	{
		lifetimeTimer_ = 0.0f;
		dissolveTimer_ = 0.0f;
		isDissolving_ = false;

		appearTime_ += safeDeltaTime;
		float threshold = 1.0f - (appearTime_ / appearDuration_);

		if (appearTime_ >= appearDuration_)
		{
			threshold = 0.0f;
			state_ = EnergyState::OnGround;
			animationTime_ = 0.0f;
		}

		material_.materialData_->dissolveThreshold = threshold;
	}
	// ユニットによる予約中または運搬中
	else if (state_ == EnergyState::Reserved || state_ == EnergyState::Carried)
	{
		lifetimeTimer_ = 0.0f;
		dissolveTimer_ = 0.0f;
		if (isDissolving_)
		{
			isDissolving_ = false;
			material_.materialData_->dissolveThreshold = 0.0f; 
		}
	}
	// 地上放置時・消去ディゾルブ処理
	else if (state_ == EnergyState::OnGround && lifetimeEnabled_)
	{
		if (!isDissolving_)
		{
			lifetimeTimer_ += safeDeltaTime;
			// 一定時間放置されたら消去ディゾルブ開始
			if (lifetimeTimer_ >= (std::max)(lifetime, 0.01f))
			{
				isDissolving_ = true;
				dissolveTimer_ = 0.0f;
			}
		}
		else
		{
			dissolveTimer_ += safeDeltaTime;
			const float safeDuration = (std::max)(dissolveDuration, 0.01f);
			const float progress = (std::clamp)(dissolveTimer_ / safeDuration, 0.0f, 1.0f);

			// 閾値を変化
			material_.materialData_->dissolveThreshold = progress;

			// 完全透明になったら非アクティブ化してプールに戻す
			if (progress >= 1.0f)
			{
				Reset();
				return;
			}
		}
	}

	// 浮遊・回転アニメーション処理
	if (state_ == EnergyState::OnGround || state_ == EnergyState::Reserved)
	{
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

	material_.materialData_->time += safeDeltaTime;

	// Specialは色相を回し続けて虹色に見せる。
	if (IsRainbow())
	{
		rainbowHue_ += safeDeltaTime * typeSettings_.rainbowSpeed;
		rainbowHue_ -= std::floor(rainbowHue_);
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

void EnergyPickup::SetLifetimeEnabled(bool enabled)
{
	if (lifetimeEnabled_ == enabled)
	{
		return;
	}

	lifetimeEnabled_ = enabled;
	lifetimeTimer_ = 0.0f;
	dissolveTimer_ = 0.0f;
	if (!lifetimeEnabled_)
	{
		isDissolving_ = false;
		if (state_ == EnergyState::OnGround)
		{
			material_.materialData_->dissolveThreshold = 0.0f;
		}
	}
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
	const Vector4 displayColor = MakeDisplayColor();
	Vector4 color = displayColor;
	if (isHighlighted_)
	{
		color.x = color.x + (1.0f - color.x) * 0.65f;
		color.y = color.y + (1.0f - color.y) * 0.65f;
		color.z = color.z + (1.0f - color.z) * 0.65f;
	}
	modelComponent_->materialData_->color = color;

	material_.materialData_->baseColor = color;
	// 虹色のときは輪郭も補色側へずらし、色の変化が分かりやすいようにする。
	material_.materialData_->rimColor = IsRainbow()
		? HsvToRgb(rainbowHue_ + 0.5f, typeSettings_.rainbowSaturation, 1.0f, typeSettings_.rimColor.w)
		: typeSettings_.rimColor;
	material_.materialData_->dissolveEdgeColor = typeSettings_.dissolveEdgeColor;

	// 色を設定
	particle_.SetColor(displayColor);
	// サイズを設定
	float eScale = scale * 1.2f;
	particle_.SetScale({ eScale, eScale, eScale });

	// 出現位置を設定
	particle_.SetEmitterPos(modelComponent_->worldTransform_.transform_.translate);

	modelComponent_->Update();
}

Vector4 EnergyPickup::MakeDisplayColor() const
{
	if (!IsRainbow())
	{
		return typeSettings_.color;
	}

	// アルファはRegisterで設定した値をそのまま使い、RGBだけ色相から作る。
	return HsvToRgb(rainbowHue_, typeSettings_.rainbowSaturation, 0.3f, typeSettings_.color.w);
}

Vector4 EnergyPickup::HsvToRgb(float hue, float saturation, float value, float alpha)
{
	// 色相は1.0で1周するため、範囲外は折り返す。
	hue -= std::floor(hue);
	saturation = (std::clamp)(saturation, 0.0f, 1.0f);
	value = (std::clamp)(value, 0.0f, 1.0f);

	const float sector = hue * 6.0f;
	const int index = static_cast<int>(sector) % 6;
	const float fraction = sector - std::floor(sector);

	const float p = value * (1.0f - saturation);
	const float q = value * (1.0f - saturation * fraction);
	const float t = value * (1.0f - saturation * (1.0f - fraction));

	switch (index)
	{
	case 0:  return { value, t, p, alpha };
	case 1:  return { q, value, p, alpha };
	case 2:  return { p, value, t, alpha };
	case 3:  return { p, q, value, alpha };
	case 4:  return { t, p, value, alpha };
	default: return { value, p, q, alpha };
	}
}

