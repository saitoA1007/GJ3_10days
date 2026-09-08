#include "LockOnController.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Camera.h"
#include "DebugRenderer.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "InputCommand.h"
#include "MyMath.h"

#include "Application/Enemy/Enemy.h"
#include "Application/Enemy/EnemyManager.h"
#include "Application/Energy/EnergyPickup.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/Field/Field.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Unit/UnitManager.h"

using namespace GameEngine;

namespace 
{
	// 登録する入力コマンド名。入力デバイスの違いはScene側で吸収する。
	constexpr const char* kCursorUpCommand = "CursorUp";
	constexpr const char* kCursorDownCommand = "CursorDown";
	constexpr const char* kCursorLeftCommand = "CursorLeft";
	constexpr const char* kCursorRightCommand = "CursorRight";
	constexpr const char* kLockOnTriggerCommand = "LockOnTrigger";
	constexpr const char* kLockOnPushCommand = "LockOnPush";
	constexpr const char* kLockOnReleaseCommand = "LockOnRelease";
	// Camera初期化時の画面サイズと合わせ、マウス座標をワールドへ逆変換する。
	constexpr float kViewportWidth = 1280.0f;
	constexpr float kViewportHeight = 720.0f;
}

LockOnController::LockOnController(
	Input* input,
	InputCommand* inputCommand,
	Camera* camera,
	Model* cursorModel,
	DebugRenderer* debugRenderer,
	Field* field,
	Rocket* rocket,
	EnergySpawner* energySpawner,
	EnemyManager* enemyManager,
	UnitManager* unitManager,
	const LockOnSettings& settings)
	: input_(input),
	inputCommand_(inputCommand),
	camera_(camera),
	debugRenderer_(debugRenderer),
	field_(field),
	rocket_(rocket),
	energySpawner_(energySpawner),
	enemyManager_(enemyManager),
	unitManager_(unitManager),
	settings_(settings) {

	// カーソルは画面上で色を判別しやすいよう、ライティングの影響を受けない。
	cursorModel_ = std::make_unique<ModelComponent>(cursorModel);
	cursorModel_->materialData_->enableLighting = false;

	chargeModel_ = std::make_unique<ModelComponent>(cursorModel);
	chargeModel_->materialData_->enableLighting = false;

	debugParameter_ = std::make_unique<DebugParameter>("LockOn");
	debugParameter_->Register("CursorSpeed", settings_.cursorSpeed, 0, "Cursor");
	debugParameter_->Register("SelectionRadius", settings_.selectionRadius, 1, "Cursor");
	debugParameter_->Register("GroundHeight", settings_.groundHeight, 2, "Cursor");
	debugParameter_->Register("FieldEdgeMargin", settings_.fieldEdgeMargin, 3, "Cursor");
	debugParameter_->Register("MouseMoveThreshold", settings_.mouseMoveThreshold, 4, "Cursor");
	debugParameter_->Register("ModelScale", settings_.cursorModelScale, 5, "Cursor");
	debugParameter_->Register("ModelHeightOffset", settings_.cursorModelHeightOffset, 6, "Cursor");
	debugParameter_->Register("MaxSeconds", settings_.maxLockOnSeconds, 0, "Charge");
	debugParameter_->Register("StartSeconds", settings_.chargeStartSeconds, 1, "Charge");
	debugParameter_->Register("MaxEnergyCost", settings_.maxChargeEnergyCost, 2, "Charge");
	debugParameter_->Register("CursorColor", settings_.cursorColor, 0, "Color");
	debugParameter_->Register("TargetColor", settings_.targetColor, 1, "Color");
	debugParameter_->Register("ChargeColor", settings_.chargeColor, 2, "Color");
	debugParameter_->Apply();
	SanitizeSettings();
	SetUpdateOrder(30);
}

void LockOnController::Initialize() 
{
	ApplyDebugParameters();
	gameplayEnabled_ = true;
	SetSelection(nullptr, nullptr);
	cursorPosition_ = rocket_->GetPosition();
	cursorPosition_.y = settings_.groundHeight;
	lockOnSeconds_ = 0.0f;
	minimumDispatchHoldSeconds_ = 0.0f;
	isCharging_ = false;
	isPositionTarget_ = false;
	enemySelectionEnabled_ = true;
	SyncCursorModel();
}

void LockOnController::Update() 
{
	ApplyDebugParameters();
	if (!gameplayEnabled_)
	{
		SyncCursorModel();
		SyncChargeModel();
		return;
	}

	UpdateCursor(FpsCounter::gameDeltaTime);

	// チャージ中は対象を固定し、それ以外のときだけ最寄り対象を探し直す。
	if (isCharging_)
	{
		UpdateLockOn(FpsCounter::gameDeltaTime);
	}
	else {
		UpdateSelection();
		if (inputCommand_->IsCommandActive(kLockOnTriggerCommand)) 
		{
			StartLockOn();
		}
	}
	SyncCursorModel();
	SyncChargeModel();
}

void LockOnController::DebugUpdate() 
{
	ApplyDebugParameters();
	SyncCursorModel();
	SyncChargeModel();
	if (gameplayEnabled_)
	{
		DrawLockOnGuide();
	}
	DrawDebugWindow();
}

void LockOnController::Draw() 
{
	if (gameplayEnabled_) 
	{
		cursorModel_->DrawRaytracing(renderQueue_);

		if (isCharging_ && (selectedEnergy_ || selectedEnemy_ || isPositionTarget_))
		{
			chargeModel_->DrawRaytracing(renderQueue_);
		}
	}
}

void LockOnController::SetGameplayEnabled(bool enabled) 
{
	if (gameplayEnabled_ == enabled) 
	{
		return;
	}

	gameplayEnabled_ = enabled;
	// 時間切れやPause中に入力を離しても、後から派遣が成立しないよう解除する。
	if (!gameplayEnabled_)
	{
		CancelLockOn();
	}
}

void LockOnController::SetMinimumDispatchHoldSeconds(float seconds)
{
	minimumDispatchHoldSeconds_ = (std::max)(seconds, 0.0f);
}

void LockOnController::SetEnemySelectionEnabled(bool enabled)
{
	if (enemySelectionEnabled_ == enabled)
	{
		return;
	}

	enemySelectionEnabled_ = enabled;
	if (!enemySelectionEnabled_ && selectedEnemy_)
	{
		CancelLockOn();
	}
}

void LockOnController::ApplyDebugParameters()
{
	debugParameter_->ApplyIfDirty();
	SanitizeSettings();
}

void LockOnController::SanitizeSettings()
{
	settings_.cursorSpeed = (std::max)(settings_.cursorSpeed, 0.0f);
	settings_.selectionRadius = (std::max)(settings_.selectionRadius, 0.0f);
	settings_.fieldEdgeMargin = (std::max)(settings_.fieldEdgeMargin, 0.0f);
	settings_.maxLockOnSeconds = (std::max)(settings_.maxLockOnSeconds, 0.01f);
	settings_.chargeStartSeconds = (std::clamp)(
		settings_.chargeStartSeconds,
		0.0f,
		settings_.maxLockOnSeconds);
	settings_.maxChargeEnergyCost = (std::max)(settings_.maxChargeEnergyCost, 0);
	settings_.mouseMoveThreshold = (std::max)(settings_.mouseMoveThreshold, 0.0f);
	settings_.cursorModelScale.x = (std::max)(settings_.cursorModelScale.x, 0.0f);
	settings_.cursorModelScale.y = (std::max)(settings_.cursorModelScale.y, 0.0f);
	settings_.cursorModelScale.z = (std::max)(settings_.cursorModelScale.z, 0.0f);
}

void LockOnController::UpdateCursor(float deltaTime) 
{
	const Vector2 mouseDelta = input_->GetMouseDelta();
	const bool mouseMoved = mouseDelta.LengthSquared() >
		settings_.mouseMoveThreshold * settings_.mouseMoveThreshold;
	// マウスが動いたフレームは絶対位置を優先し、キー・スティックとの競合を防ぐ。
	if (mouseMoved && TrySetCursorFromMouse()) 
	{
		ClampCursorToField();
		return;
	}

	// WASDと左スティックを同じ2D移動ベクトルへ合成する。
	Vector2 move = inputCommand_->GetLeftStick();
	if (inputCommand_->IsCommandActive(kCursorLeftCommand)) 
	{
		move.x -= 1.0f;
	}
	if (inputCommand_->IsCommandActive(kCursorRightCommand)) 
	{
		move.x += 1.0f;
	}
	if (inputCommand_->IsCommandActive(kCursorUpCommand))
	{
		move.y += 1.0f;
	}
	if (inputCommand_->IsCommandActive(kCursorDownCommand))
	{
		move.y -= 1.0f;
	}

	if (move.LengthSquared() > 1.0f) 
	{
		move.Normalize();
	}
	cursorPosition_.x += move.x * settings_.cursorSpeed * (std::max)(deltaTime, 0.0f);
	cursorPosition_.z += move.y * settings_.cursorSpeed * (std::max)(deltaTime, 0.0f);
	cursorPosition_.y = settings_.groundHeight;
	ClampCursorToField();
}

bool LockOnController::TrySetCursorFromMouse() 
{
	const Vector2 mousePosition = input_->GetMousePosition();
	// Near/Farのスクリーン座標を逆変換し、カメラから地面へ伸びるレイを作る。
	const Matrix4x4 viewport = Math::MakeViewportMatrix(
		0.0f,
		0.0f,
		kViewportWidth,
		kViewportHeight,
		0.0f,
		1.0f);
	const Matrix4x4 screenToWorld = Math::InverseMatrix(camera_->GetVPMatrix() * viewport);
	const Vector3 nearPoint = Math::Transforms({ mousePosition.x, mousePosition.y, 0.0f }, screenToWorld);
	const Vector3 farPoint = Math::Transforms({ mousePosition.x, mousePosition.y, 1.0f }, screenToWorld);
	const Vector3 ray = farPoint - nearPoint;

	if (std::abs(ray.y) <= 0.0001f)
	{
		return false;
	}
	// レイとY=groundHeight平面の交点距離を求める。
	const float distance = (settings_.groundHeight - nearPoint.y) / ray.y;
	if (distance < 0.0f) 
	{
		return false;
	}

	cursorPosition_ = nearPoint + ray * distance;
	cursorPosition_.y = settings_.groundHeight;
	return true;
}

void LockOnController::ClampCursorToField() 
{
	const Vector3 center = field_->GetSettings().center;
	const float fieldRadius = field_->GetRadius(FieldZone::OuterBuffer);
	const float allowedRadius = (std::max)(fieldRadius - settings_.fieldEdgeMargin, 0.0f);
	const float offsetX = cursorPosition_.x - center.x;
	const float offsetZ = cursorPosition_.z - center.z;
	const float distanceSquared = offsetX * offsetX + offsetZ * offsetZ;

	if (distanceSquared > allowedRadius * allowedRadius && distanceSquared > 0.0f) {
		// 中心からの方向は維持し、半径だけを許可範囲まで縮める。
		const float scale = allowedRadius / std::sqrt(distanceSquared);
		cursorPosition_.x = center.x + offsetX * scale;
		cursorPosition_.z = center.z + offsetZ * scale;
	}
}

void LockOnController::SyncCursorModel() 
{
	// 選択範囲と見た目の大きさを一致させ、ModelScaleはモデル固有の補正倍率として使う。
	cursorModel_->worldTransform_.transform_.scale = 
	{
		settings_.cursorModelScale.x * settings_.selectionRadius,
		settings_.cursorModelScale.y * settings_.selectionRadius,
		settings_.cursorModelScale.z * settings_.selectionRadius,
	};
	cursorModel_->worldTransform_.transform_.translate =
	{
		cursorPosition_.x,
		cursorPosition_.y + settings_.cursorModelHeightOffset,
		cursorPosition_.z,
	};
	cursorModel_->materialData_->color = settings_.cursorColor;
	cursorModel_->Update();
}

void LockOnController::SyncChargeModel()
{
	if (!isCharging_ || (!selectedEnergy_ && !selectedEnemy_ && !isPositionTarget_))
	{
		return;
	}

	Vector3 targetPosition = isPositionTarget_
		? positionTarget_
		: (selectedEnemy_ ? selectedEnemy_->GetPosition() : selectedEnergy_->GetPosition());

	const float targetRadius = isPositionTarget_
		? settings_.selectionRadius
		: (selectedEnemy_ ? selectedEnemy_->GetDisplayScale() + 0.25f : selectedEnergy_->GetScale() + 0.25f);

	const float ratio = CalculateChargeRatio();
	const float currentRadius = targetRadius + ratio * settings_.selectionRadius;

	chargeModel_->worldTransform_.transform_.scale =
	{
		settings_.cursorModelScale.x * currentRadius,
		targetPosition.y + settings_.cursorModelHeightOffset + 0.01f,
		settings_.cursorModelScale.z * currentRadius,
	};

	chargeModel_->worldTransform_.transform_.translate =
	{
		targetPosition.x,
		settings_.groundHeight + settings_.cursorModelHeightOffset + 0.01f,
		targetPosition.z,
	};

	chargeModel_->materialData_->color = settings_.chargeColor;
	chargeModel_->Update();
}

void LockOnController::UpdateSelection()
{
	EnergyPickup* energy = energySpawner_->FindNearestAvailable(cursorPosition_, settings_.selectionRadius);
	Enemy* enemy = enemySelectionEnabled_
		? enemyManager_->FindNearestTargetable(cursorPosition_, settings_.selectionRadius)
		: nullptr;

	if (energy && enemy) {
		// 両方が範囲内なら距離で比較し、同距離では防衛を優先してEnemyを選ぶ。
		const Vector3 energyOffset = energy->GetPosition() - cursorPosition_;
		const Vector3 enemyOffset = enemy->GetPosition() - cursorPosition_;
		const float energyDistanceSquared = energyOffset.x * energyOffset.x + energyOffset.z * energyOffset.z;
		const float enemyDistanceSquared = enemyOffset.x * enemyOffset.x + enemyOffset.z * enemyOffset.z;
		if (enemyDistanceSquared <= energyDistanceSquared)
		{
			energy = nullptr;
		}
		else {
			enemy = nullptr;
		}
	}

	SetSelection(energy, enemy);
}

void LockOnController::StartLockOn()
{
	// 空地点も派遣先にできるため、対象の有無にかかわらず待機Unitがいれば開始する。
	if (unitManager_->GetAvailableCount() == 0)
	{
		return;
	}

	isPositionTarget_ = !HasValidSelection();
	positionTarget_ = cursorPosition_;
	isCharging_ = true;
	lockOnSeconds_ = 0.0f;
	chargedEnergy_ = 0;
}

void LockOnController::UpdateLockOn(float deltaTime)
{
	// 対象が他処理で消えた場合はEnergyを消費せずキャンセル
	if (!isPositionTarget_ && !HasValidSelection()) {
		CancelLockOn();
		return;
	}

	if (inputCommand_->IsCommandActive(kLockOnPushCommand))
	{
		// 無条件でチャージ時間を進める
		lockOnSeconds_ = (std::min)(
			lockOnSeconds_ + (std::max)(deltaTime, 0.0f),
			settings_.maxLockOnSeconds);

		// 加算後の時間に応じた目標エネルギーと不足分を計算
		const int32_t targetEnergy = CalculateRequestedEnergy();
		const int32_t neededEnergy = targetEnergy - chargedEnergy_;

		// 新たにエネルギーが必要になった場合のみ引き落とす
		if (neededEnergy > 0)
		{
			const EnergyChange change = rocket_->AllocateEnergyToUnit(neededEnergy);

			// 消費時は amount が負数になるため絶対値を取得
			const int32_t actualAllocated = std::abs(change.amount);
			chargedEnergy_ += actualAllocated;

			// ロケットのエネルギー不足で要求量より引き落とせなかった場合
			if (actualAllocated < neededEnergy)
			{
				// 実際に確保できた chargedEnergy_ の量に応じた時間へ逆算して固定
				const float chargeDuration = settings_.maxLockOnSeconds - settings_.chargeStartSeconds;
				if (settings_.maxChargeEnergyCost > 0 && chargeDuration > 0.0f)
				{
					const float ratio = static_cast<float>(chargedEnergy_) / static_cast<float>(settings_.maxChargeEnergyCost);
					lockOnSeconds_ = settings_.chargeStartSeconds + ratio * chargeDuration;
				}
			}
		}
	}

	// 離した瞬間に現在のチャージ量を確定し、1体だけ派遣する。
	if (inputCommand_->IsCommandActive(kLockOnReleaseCommand))
	{
		CompleteLockOn();
	}
}

void LockOnController::CompleteLockOn()
{
	if (lockOnSeconds_ < minimumDispatchHoldSeconds_)
	{
		CancelLockOn();
		UpdateSelection();
		return;
	}

	// リアルタイムで引き落とした chargedEnergy_ をそのまま Unit に渡す
	bool dispatched = false;
	if (selectedEnergy_)
	{
		dispatched = unitManager_->DispatchToEnergy(selectedEnergy_, chargedEnergy_);
	}
	else if (selectedEnemy_)
	{
		dispatched = unitManager_->DispatchToEnemy(selectedEnemy_, chargedEnergy_);
	}
	else if (isPositionTarget_)
	{
		dispatched = unitManager_->DispatchToPosition(positionTarget_, chargedEnergy_);
	}

	// 派遣に失敗した場合は引き落としたエネルギーをロケットに返却
	if (!dispatched && chargedEnergy_ > 0)
	{
		rocket_->DepositEnergy(chargedEnergy_);
	}

	chargedEnergy_ = 0;
	SetSelection(nullptr, nullptr);
	isCharging_ = false;
	isPositionTarget_ = false;
	lockOnSeconds_ = 0.0f;

	if (!dispatched)
	{
		UpdateSelection();
	}
}

void LockOnController::CancelLockOn()
{
	if (chargedEnergy_ > 0 && rocket_)
	{
		rocket_->DepositEnergy(chargedEnergy_);
	}

	chargedEnergy_ = 0;
	isCharging_ = false;
	isPositionTarget_ = false;
	lockOnSeconds_ = 0.0f;
	SetSelection(nullptr, nullptr);
}

float LockOnController::CalculateChargeRatio() const
{
	if (lockOnSeconds_ >= settings_.maxLockOnSeconds) 
	{
		return 1.0f;
	}
	// 短押し猶予内は必ず0とし、単発クリックによる1Energy消費を防ぐ。
	if (lockOnSeconds_ <= settings_.chargeStartSeconds) 
	{
		return 0.0f;
	}

	const float chargeDuration = settings_.maxLockOnSeconds - settings_.chargeStartSeconds;
	if (chargeDuration <= 0.0f)
	{
		return 0.0f;
	}

	return (std::clamp)(
		(lockOnSeconds_ - settings_.chargeStartSeconds) / chargeDuration,
		0.0f,
		1.0f);
}

int32_t LockOnController::CalculateRequestedEnergy() const 
{
	// 正数のstatic_castは小数点以下を切り捨てるため、低チャージの繰り上がりがない。
	return static_cast<int32_t>(
		static_cast<float>(settings_.maxChargeEnergyCost) * CalculateChargeRatio());
}

bool LockOnController::HasValidSelection() const
{
	return (selectedEnergy_ && selectedEnergy_->IsTargetable()) ||
		(enemySelectionEnabled_ && selectedEnemy_ && selectedEnemy_->IsTargetable());
}

void LockOnController::SetSelection(EnergyPickup* energy, Enemy* enemy)
{
	if (selectedEnergy_ == energy && selectedEnemy_ == enemy)
	{
		return;
	}
	// 前の対象を通常色へ戻してから、新しい対象だけを強調する。
	if (selectedEnergy_)
	{
		selectedEnergy_->SetHighlighted(false);
	}
	if (selectedEnemy_) 
	{
		selectedEnemy_->SetHighlighted(false);
	}

	selectedEnergy_ = energy;
	selectedEnemy_ = enemy;
	if (selectedEnergy_)
	{
		selectedEnergy_->SetHighlighted(true);
	}
	if (selectedEnemy_)
	{
		selectedEnemy_->SetHighlighted(true);
	}
}

void LockOnController::DrawLockOnGuide()
{
	if (!debugRenderer_) return;

	// 通常モデルとは別に、選択半径・対象・派遣経路をデバッグ線で確認できるようにする。
	debugRenderer_->AddCircle(
		cursorPosition_,
		{ 0.0f, 1.0f, 0.0f },
		settings_.selectionRadius,
		settings_.cursorColor,
		32);

	if (!selectedEnergy_ && !selectedEnemy_ && !isPositionTarget_)
	{
		return;
	}

	// 選択中対象（敵優先）の位置と半径を取得
	Vector3 targetPosition = isPositionTarget_
		? positionTarget_
		: (selectedEnemy_ ? selectedEnemy_->GetPosition() : selectedEnergy_->GetPosition());
	targetPosition.y += 0.08f;

	const float targetRadius = isPositionTarget_
		? settings_.selectionRadius
		: (selectedEnemy_ ? selectedEnemy_->GetDisplayScale() + 0.25f : selectedEnergy_->GetScale() + 0.25f);

	debugRenderer_->AddCircle(
		targetPosition,
		{ 0.0f, 1.0f, 0.0f },
		targetRadius,
		settings_.targetColor,
		32);

	if (rocket_)
	{
		debugRenderer_->AddLine(rocket_->GetPosition(), targetPosition, settings_.targetColor);
	}

	if (isCharging_)
	{
		const float ratio = CalculateChargeRatio();
		debugRenderer_->AddCircle(
			targetPosition,
			{ 0.0f, 1.0f, 0.0f },
			targetRadius + ratio * settings_.selectionRadius,
			settings_.chargeColor,
			32);
	}
}

void LockOnController::DrawDebugWindow() 
{
#ifdef USE_IMGUI
	if (!ImGui::Begin("LockOn"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Move: Mouse / WASD / Left Stick");
	ImGui::Text("LockOn: Left Click / Space / Pad A");
	ImGui::Text("Available Units: %zu", unitManager_->GetAvailableCount());
	ImGui::Text("Rocket Energy: %d", rocket_->GetEnergy());

	const char* selectedType = selectedEnemy_ ? "Enemy" : (selectedEnergy_ ? "Energy" : (isPositionTarget_ ? "Position" : "None"));
	ImGui::Text("Selected: %s", selectedType);

	ImGui::Text("Charge: %.2f / %.2f sec", lockOnSeconds_, settings_.maxLockOnSeconds);
	const float chargeRatio = CalculateChargeRatio();
	ImGui::Text("Charge Ratio: %.0f%%", chargeRatio * 100.0f);
	ImGui::Text("Requested Energy: %d / %d", CalculateRequestedEnergy(), settings_.maxChargeEnergyCost);

	ImGui::End();
#endif
}

