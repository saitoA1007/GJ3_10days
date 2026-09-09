#include "Rocket.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "Application/CollisionConfig.h"
#include "AudioManager.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
#include "Application/Field/FieldEffect.h"

using namespace GameEngine;

namespace
{
	constexpr const char* kDescentSoundName = "rocketDescent.mp3";
	constexpr const char* kLandingSoundName = "rocketLanding.mp3";
	constexpr const char* kEnergyChargeSoundName = "energyCharge.mp3";
	constexpr float kEntranceSoundVolume = 1.0f;
	constexpr float kEnergyChargeSoundVolume = 1.0f;
}

Rocket::Rocket(Model* model, FieldEffect* fieldEffect, const RocketSettings& settings)
	: settings_(settings), energy_(settings.initialEnergy)
{
	fieldEffect_ = fieldEffect;

	assert(model != nullptr && "rocket requires Rocket.gltf");
	modelComponent_ = std::make_unique<ModelComponent>(model);
	modelComponent_->materialData_->enableLighting = true;

	// RocketはEnemyだけを受け付け、接触時のEnergy減算をコールバックへ集約する。
	collider_.SetCollisionAttribute(kCollisionAttributeRocket);
	collider_.SetCollisionMask(kCollisionAttributeEnemy);
	collider_.SetUserData({ static_cast<uint32_t>(CollisionTypeID::kRocket), this });
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) 
		{
		OnCollisionEnter(result);
		});

	debugParameter_ = std::make_unique<DebugParameter>("Rocket");
	debugParameter_->Register("Scale", settings_.scale, 0, "Transform");
	debugParameter_->Register("Rotate", settings_.rotation, 1, "Transform");
	debugParameter_->Register("Enabled", settings_.entranceEnabled, 0, "Entrance");
	debugParameter_->Register("StartPosition", settings_.entranceStartPosition, 1, "Entrance");
	debugParameter_->Register("EndPosition", settings_.position, 2, "Entrance");
	debugParameter_->Register("Duration", settings_.entranceDuration, 3, "Entrance");
	debugParameter_->Register("EaseType", settings_.entranceEaseType, 4, "Entrance");
	debugParameter_->Register("Radius", settings_.colliderRadius, 0, "Collider");
	debugParameter_->Register("OffsetY", settings_.colliderOffsetY, 1, "Collider");
	debugParameter_->Register("InitialEnergy", settings_.initialEnergy, 0, "Energy");
	debugParameter_->Register("EnemyHitLoss", settings_.enemyHitLoss, 1, "Energy");
	debugParameter_->Register("ChangeAmount", settings_.debugEnergyAmount, 0, "Debug");
	debugParameter_->Apply();
	SanitizeSettings();
}

void Rocket::Initialize() 
{
	ApplyDebugParameters();
	energy_.Reset(settings_.initialEnergy);
	enemyHitCount_ = 0;
	collider_.SetActive(true);
	StartEntrance();
	SyncComponents();
}

void Rocket::Update() 
{
	ApplyDebugParameters();
	UpdateEntrance(FpsCounter::gameDeltaTime);
	SyncComponents();
}

void Rocket::DebugUpdate() 
{
	ApplyDebugParameters();
	UpdateEntrance(0.0f);
	SyncComponents();
	DrawDebugWindow();
}

void Rocket::Draw() 
{
	modelComponent_->DrawRaytracing(renderQueue_);
}

void Rocket::StartEntrance()
{
	auto& audioManager = AudioManager::GetInstance();
	const uint32_t descentSoundHandle = audioManager.GetHandleByName(kDescentSoundName);
	const uint32_t landingSoundHandle = audioManager.GetHandleByName(kLandingSoundName);
	// シーン再初期化やReplay時に、前回の登場音が重ならないようにする。
	audioManager.Stop(descentSoundHandle);
	audioManager.Stop(landingSoundHandle);

	entranceElapsedTime_ = 0.0f;
	if (settings_.entranceEnabled && settings_.entranceDuration > 0.0f)
	{
		currentPosition_ = settings_.entranceStartPosition;
		isEntrancePlaying_ = true;
		audioManager.Play(descentSoundHandle, kEntranceSoundVolume, false);
		return;
	}

	currentPosition_ = settings_.position;
	isEntrancePlaying_ = false;
}

float Rocket::GetEntranceProgress() const
{
	if (!settings_.entranceEnabled || settings_.entranceDuration <= 0.0f || !isEntrancePlaying_)
	{
		return 1.0f;
	}

	const float progress = (std::clamp)(
		entranceElapsedTime_ / settings_.entranceDuration,
		0.0f,
		1.0f);
	return GameEngine::Apply(progress, settings_.entranceEaseType);
}

EnergyChange Rocket::DepositEnergy(int32_t amount) 
{
	// 増減理由を付けておくことで、将来UIや演出が変化元を判別できる。
	const EnergyChange change = energy_.Add(amount, EnergyChangeReason::Delivery);
	NotifyEnergyChanged(change);
	if (change.Changed())
	{
		auto& audioManager = AudioManager::GetInstance();
		const uint32_t energyChargeSoundHandle =
			audioManager.GetHandleByName(kEnergyChargeSoundName);
		audioManager.Stop(energyChargeSoundHandle);
		audioManager.Play(energyChargeSoundHandle, kEnergyChargeSoundVolume, false);
	}
	// 取得したことによるフィールド演出
	fieldEffect_->Start({0.0f,1.0f,0.137f,1.0f});
	return change;
}

EnergyChange Rocket::AllocateEnergyToUnit(int32_t requestedAmount)
{
	// 不足時は残量だけを渡す。Unitは返された実消費量をスタミナとして使う。
	const EnergyChange change = energy_.ConsumeUpTo(requestedAmount, EnergyChangeReason::UnitAllocation);
	NotifyEnergyChanged(change);
	return change;
}

EnergyChange Rocket::ReceiveEnemyHit() 
{
	++enemyHitCount_;
	const EnergyChange change = energy_.ConsumeUpTo(settings_.enemyHitLoss, EnergyChangeReason::EnemyHit);
	NotifyEnergyChanged(change);
	// ダメージを受けたことによるフィールド演出
	fieldEffect_->Start({ 1.0f,0.0f,0.0f,1.0f });
	return change;
}

void Rocket::ResetEnergy()
{
	NotifyEnergyChanged(energy_.Reset(settings_.initialEnergy));
}

void Rocket::ApplyDebugParameters()
{
	debugParameter_->ApplyIfDirty();
	SanitizeSettings();
}

void Rocket::SanitizeSettings()
{
	settings_.scale.x = (std::max)(settings_.scale.x, 0.0f);
	settings_.scale.y = (std::max)(settings_.scale.y, 0.0f);
	settings_.scale.z = (std::max)(settings_.scale.z, 0.0f);
	settings_.entranceDuration = (std::max)(settings_.entranceDuration, 0.0f);
	settings_.colliderRadius = (std::max)(settings_.colliderRadius, 0.0f);
	settings_.initialEnergy = (std::max)(settings_.initialEnergy, 0);
	settings_.enemyHitLoss = (std::max)(settings_.enemyHitLoss, 0);
	settings_.debugEnergyAmount = (std::max)(settings_.debugEnergyAmount, 0);
}

void Rocket::UpdateEntrance(float deltaTime)
{
	// 無効化または0秒設定では、常にB座標へ即時反映する。
	if (!settings_.entranceEnabled || settings_.entranceDuration <= 0.0f)
	{
		if (isEntrancePlaying_)
		{
			auto& audioManager = AudioManager::GetInstance();
			audioManager.Stop(audioManager.GetHandleByName(kDescentSoundName));
		}
		currentPosition_ = settings_.position;
		isEntrancePlaying_ = false;
		return;
	}

	// 再生完了後にB座標を調整した場合も、その場で反映する。
	if (!isEntrancePlaying_)
	{
		currentPosition_ = settings_.position;
		return;
	}

	entranceElapsedTime_ = (std::min)(
		entranceElapsedTime_ + (std::max)(deltaTime, 0.0f),
		settings_.entranceDuration);
	const float progress = entranceElapsedTime_ / settings_.entranceDuration;
	currentPosition_ = GameEngine::Lerp(
		settings_.entranceStartPosition,
		settings_.position,
		progress,
		settings_.entranceEaseType);

	// 降下音は登場演出の進行に合わせ、100%から着地直前の0%まで線形にフェードアウトする。
	auto& audioManager = AudioManager::GetInstance();
	const uint32_t descentSoundHandle = audioManager.GetHandleByName(kDescentSoundName);
	audioManager.SetVolume(descentSoundHandle, kEntranceSoundVolume * (1.0f - progress));

	if (entranceElapsedTime_ >= settings_.entranceDuration)
	{
		// イージングの計算誤差を残さず、最終フレームはB座標に固定する。
		currentPosition_ = settings_.position;
		isEntrancePlaying_ = false;

		audioManager.Stop(descentSoundHandle);
		const uint32_t landingSoundHandle = audioManager.GetHandleByName(kLandingSoundName);
		audioManager.Stop(landingSoundHandle);
		audioManager.Play(landingSoundHandle, kEntranceSoundVolume, false);
	}
}

void Rocket::SyncComponents() 
{
	// 見た目と当たり判定が別座標にならないよう、同じ設定から毎回同期する。
	modelComponent_->worldTransform_.transform_.scale = settings_.scale;
	modelComponent_->worldTransform_.transform_.rotate = settings_.rotation;
	modelComponent_->worldTransform_.transform_.translate = currentPosition_;
	modelComponent_->Update();

	collider_.SetWorldPosition({
		currentPosition_.x,
		currentPosition_.y + settings_.colliderOffsetY,
		currentPosition_.z,
		});
	collider_.SetRadius(settings_.colliderRadius);
}

void Rocket::NotifyEnergyChanged(const EnergyChange& change)
{
	// 0消費・0加算では不要なUI更新を発生させない。
	if (change.Changed() && onEnergyChanged_)
	{
		onEnergyChanged_(change);
	}
}

void Rocket::OnCollisionEnter(const CollisionResult& result)
{
	if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy)) 
	{
		ReceiveEnemyHit();
	}
}

void Rocket::DrawDebugWindow()
{
#ifdef USE_IMGUI
	if (!ImGui::Begin("Rocket"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Energy: %d", energy_.GetCurrent());
	const float entranceProgress = settings_.entranceDuration > 0.0f
		? (std::clamp)(entranceElapsedTime_ / settings_.entranceDuration, 0.0f, 1.0f)
		: 1.0f;
	ImGui::Text(
		"Entrance: %s (%.0f%%)",
		!settings_.entranceEnabled ? "Disabled" : (isEntrancePlaying_ ? "Playing" : "Finished"),
		entranceProgress * 100.0f);
	if (ImGui::Button("Replay Entrance"))
	{
		StartEntrance();
	}
	ImGui::TextDisabled("ChangeAmount is configured in ParameterInspector.");

	if (ImGui::Button("Deposit")) 
	{
		const EnergyChange change = energy_.Add(settings_.debugEnergyAmount, EnergyChangeReason::Debug);
		NotifyEnergyChanged(change);
	}
	ImGui::SameLine();
	if (ImGui::Button("Consume")) 
	{
		const EnergyChange change = energy_.ConsumeUpTo(settings_.debugEnergyAmount, EnergyChangeReason::Debug);
		NotifyEnergyChanged(change);
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		ResetEnergy();
	}

	ImGui::End();
#endif
}

