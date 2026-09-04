#include "PrototypeLockOnController.h"

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

#include "Application/Prototype/Enemy/PrototypeEnemy.h"
#include "Application/Prototype/Enemy/PrototypeEnemyManager.h"
#include "Application/Prototype/Energy/PrototypeEnergyPickup.h"
#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/Field/PrototypeField.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"

using namespace GameEngine;

namespace {
	constexpr const char* kCursorUpCommand = "ProtoCursorUp";
	constexpr const char* kCursorDownCommand = "ProtoCursorDown";
	constexpr const char* kCursorLeftCommand = "ProtoCursorLeft";
	constexpr const char* kCursorRightCommand = "ProtoCursorRight";
	constexpr const char* kLockOnTriggerCommand = "ProtoLockOnTrigger";
	constexpr const char* kLockOnPushCommand = "ProtoLockOnPush";
	constexpr const char* kLockOnReleaseCommand = "ProtoLockOnRelease";
	constexpr float kViewportWidth = 1280.0f;
	constexpr float kViewportHeight = 720.0f;
}

namespace Prototype {

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
		assert(input_ != nullptr && "Prototype lock-on requires input");
		assert(inputCommand_ != nullptr && "Prototype lock-on requires input commands");
		assert(camera_ != nullptr && "Prototype lock-on requires a camera");
		assert(cursorModel != nullptr && "Prototype lock-on requires cursor.obj");
		assert(debugRenderer_ != nullptr && "Prototype lock-on requires a debug renderer");
		assert(field_ != nullptr && "Prototype lock-on requires a field");
		assert(rocket_ != nullptr && "Prototype lock-on requires a rocket");
		assert(energySpawner_ != nullptr && "Prototype lock-on requires an energy spawner");
		assert(enemyManager_ != nullptr && "Prototype lock-on requires an enemy manager");
		assert(unitManager_ != nullptr && "Prototype lock-on requires a unit manager");

		cursorModel_ = std::make_unique<ModelComponent>(cursorModel);
		cursorModel_->materialData_->enableLighting = false;

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeLockOn");
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

	void LockOnController::Initialize() {
		ApplyDebugParameters();
		gameplayEnabled_ = true;
		SetSelection(nullptr, nullptr);
		cursorPosition_ = rocket_->GetPosition();
		cursorPosition_.y = settings_.groundHeight;
		lockOnSeconds_ = 0.0f;
		isCharging_ = false;
		SyncCursorModel();
	}

	void LockOnController::Update() {
		ApplyDebugParameters();
		if (!gameplayEnabled_) {
			SyncCursorModel();
			return;
		}

		UpdateCursor(FpsCounter::gameDeltaTime);

		if (isCharging_) {
			UpdateLockOn(FpsCounter::gameDeltaTime);
		} else {
			UpdateSelection();
			if (inputCommand_->IsCommandActive(kLockOnTriggerCommand)) {
				StartLockOn();
			}
		}
		SyncCursorModel();
	}

	void LockOnController::DebugUpdate() {
		ApplyDebugParameters();
		SyncCursorModel();
		if (gameplayEnabled_) {
			DrawLockOnGuide();
		}
		DrawDebugWindow();
	}

	void LockOnController::Draw() {
		if (gameplayEnabled_) {
			cursorModel_->DrawRaytracing(renderQueue_);
		}
	}

	void LockOnController::SetGameplayEnabled(bool enabled) {
		if (gameplayEnabled_ == enabled) {
			return;
		}

		gameplayEnabled_ = enabled;
		if (!gameplayEnabled_) {
			CancelLockOn();
		}
	}

	void LockOnController::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void LockOnController::SanitizeSettings() {
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

	void LockOnController::UpdateCursor(float deltaTime) {
		const Vector2 mouseDelta = input_->GetMouseDelta();
		const bool mouseMoved = mouseDelta.LengthSquared() >
			settings_.mouseMoveThreshold * settings_.mouseMoveThreshold;
		if (mouseMoved && TrySetCursorFromMouse()) {
			ClampCursorToField();
			return;
		}

		Vector2 move = inputCommand_->GetLeftStick();
		if (inputCommand_->IsCommandActive(kCursorLeftCommand)) {
			move.x -= 1.0f;
		}
		if (inputCommand_->IsCommandActive(kCursorRightCommand)) {
			move.x += 1.0f;
		}
		if (inputCommand_->IsCommandActive(kCursorUpCommand)) {
			move.y += 1.0f;
		}
		if (inputCommand_->IsCommandActive(kCursorDownCommand)) {
			move.y -= 1.0f;
		}

		if (move.LengthSquared() > 1.0f) {
			move.Normalize();
		}
		cursorPosition_.x += move.x * settings_.cursorSpeed * (std::max)(deltaTime, 0.0f);
		cursorPosition_.z += move.y * settings_.cursorSpeed * (std::max)(deltaTime, 0.0f);
		cursorPosition_.y = settings_.groundHeight;
		ClampCursorToField();
	}

	bool LockOnController::TrySetCursorFromMouse() {
		const Vector2 mousePosition = input_->GetMousePosition();
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

		if (std::abs(ray.y) <= 0.0001f) {
			return false;
		}
		const float distance = (settings_.groundHeight - nearPoint.y) / ray.y;
		if (distance < 0.0f) {
			return false;
		}

		cursorPosition_ = nearPoint + ray * distance;
		cursorPosition_.y = settings_.groundHeight;
		return true;
	}

	void LockOnController::ClampCursorToField() {
		const Vector3 center = field_->GetSettings().center;
		const float fieldRadius = field_->GetRadius(FieldZone::OuterBuffer);
		const float allowedRadius = (std::max)(fieldRadius - settings_.fieldEdgeMargin, 0.0f);
		const float offsetX = cursorPosition_.x - center.x;
		const float offsetZ = cursorPosition_.z - center.z;
		const float distanceSquared = offsetX * offsetX + offsetZ * offsetZ;

		if (distanceSquared > allowedRadius * allowedRadius && distanceSquared > 0.0f) {
			const float scale = allowedRadius / std::sqrt(distanceSquared);
			cursorPosition_.x = center.x + offsetX * scale;
			cursorPosition_.z = center.z + offsetZ * scale;
		}
	}

	void LockOnController::SyncCursorModel() {
		cursorModel_->worldTransform_.transform_.scale = settings_.cursorModelScale;
		cursorModel_->worldTransform_.transform_.translate = {
			cursorPosition_.x,
			cursorPosition_.y + settings_.cursorModelHeightOffset,
			cursorPosition_.z,
		};
		cursorModel_->materialData_->color = settings_.cursorColor;
		cursorModel_->Update();
	}

	void LockOnController::UpdateSelection() {
		EnergyPickup* energy = energySpawner_->FindNearestAvailable(cursorPosition_, settings_.selectionRadius);
		Enemy* enemy = enemyManager_->FindNearestTargetable(cursorPosition_, settings_.selectionRadius);

		if (energy && enemy) {
			const Vector3 energyOffset = energy->GetPosition() - cursorPosition_;
			const Vector3 enemyOffset = enemy->GetPosition() - cursorPosition_;
			const float energyDistanceSquared = energyOffset.x * energyOffset.x + energyOffset.z * energyOffset.z;
			const float enemyDistanceSquared = enemyOffset.x * enemyOffset.x + enemyOffset.z * enemyOffset.z;
			if (enemyDistanceSquared <= energyDistanceSquared) {
				energy = nullptr;
			} else {
				enemy = nullptr;
			}
		}

		SetSelection(energy, enemy);
	}

	void LockOnController::StartLockOn() {
		if (!HasValidSelection() || unitManager_->GetAvailableCount() == 0) {
			return;
		}

		isCharging_ = true;
		lockOnSeconds_ = 0.0f;
	}

	void LockOnController::UpdateLockOn(float deltaTime) {
		if (!HasValidSelection()) {
			CancelLockOn();
			return;
		}

		if (inputCommand_->IsCommandActive(kLockOnPushCommand)) {
			lockOnSeconds_ = (std::min)(
				lockOnSeconds_ + (std::max)(deltaTime, 0.0f),
				settings_.maxLockOnSeconds);
		}

		if (inputCommand_->IsCommandActive(kLockOnReleaseCommand)) {
			CompleteLockOn();
		}
	}

	void LockOnController::CompleteLockOn() {
		const int32_t requestedEnergy = CalculateRequestedEnergy();
		const bool dispatched = selectedEnemy_
			? unitManager_->DispatchToEnemy(selectedEnemy_, requestedEnergy)
			: unitManager_->DispatchToEnergy(selectedEnergy_, requestedEnergy);
		SetSelection(nullptr, nullptr);
		isCharging_ = false;
		lockOnSeconds_ = 0.0f;

		if (!dispatched) {
			UpdateSelection();
		}
	}

	void LockOnController::CancelLockOn() {
		isCharging_ = false;
		lockOnSeconds_ = 0.0f;
		SetSelection(nullptr, nullptr);
	}

	float LockOnController::CalculateChargeRatio() const {
		if (lockOnSeconds_ >= settings_.maxLockOnSeconds) {
			return 1.0f;
		}
		if (lockOnSeconds_ <= settings_.chargeStartSeconds) {
			return 0.0f;
		}

		const float chargeDuration = settings_.maxLockOnSeconds - settings_.chargeStartSeconds;
		if (chargeDuration <= 0.0f) {
			return 0.0f;
		}

		return (std::clamp)(
			(lockOnSeconds_ - settings_.chargeStartSeconds) / chargeDuration,
			0.0f,
			1.0f);
	}

	int32_t LockOnController::CalculateRequestedEnergy() const {
		return static_cast<int32_t>(
			static_cast<float>(settings_.maxChargeEnergyCost) * CalculateChargeRatio());
	}

	bool LockOnController::HasValidSelection() const {
		return (selectedEnergy_ && selectedEnergy_->IsTargetable()) ||
			(selectedEnemy_ && selectedEnemy_->IsTargetable());
	}

	void LockOnController::SetSelection(EnergyPickup* energy, Enemy* enemy) {
		if (selectedEnergy_ == energy && selectedEnemy_ == enemy) {
			return;
		}
		if (selectedEnergy_) {
			selectedEnergy_->SetHighlighted(false);
		}
		if (selectedEnemy_) {
			selectedEnemy_->SetHighlighted(false);
		}

		selectedEnergy_ = energy;
		selectedEnemy_ = enemy;
		if (selectedEnergy_) {
			selectedEnergy_->SetHighlighted(true);
		}
		if (selectedEnemy_) {
			selectedEnemy_->SetHighlighted(true);
		}
	}

	void LockOnController::DrawLockOnGuide() {
		debugRenderer_->AddCircle(
			cursorPosition_,
			{ 0.0f, 1.0f, 0.0f },
			settings_.selectionRadius,
			settings_.cursorColor,
			32);

		if (!selectedEnergy_ && !selectedEnemy_) {
			return;
		}

		Vector3 targetPosition = selectedEnemy_
			? selectedEnemy_->GetPosition()
			: selectedEnergy_->GetPosition();
		targetPosition.y += 0.08f;
		const float targetRadius = selectedEnemy_
			? selectedEnemy_->GetDisplayScale() + 0.25f
			: selectedEnergy_->GetScale() + 0.25f;
		debugRenderer_->AddCircle(
			targetPosition,
			{ 0.0f, 1.0f, 0.0f },
			targetRadius,
			settings_.targetColor,
			32);
		debugRenderer_->AddLine(rocket_->GetPosition(), targetPosition, settings_.targetColor);

		if (isCharging_) {
			const float ratio = CalculateChargeRatio();
			debugRenderer_->AddCircle(
				targetPosition,
				{ 0.0f, 1.0f, 0.0f },
				targetRadius + ratio * settings_.selectionRadius,
				settings_.chargeColor,
				32);
		}
	}

	void LockOnController::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype LockOn")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Move: Mouse / WASD / Left Stick");
		ImGui::Text("LockOn: Left Click / Space / Pad A");
		ImGui::Text("Available Units: %zu", unitManager_->GetAvailableCount());
		ImGui::Text("Rocket Energy: %d", rocket_->GetEnergy());
		const char* selectedType = selectedEnemy_ ? "Enemy" : (selectedEnergy_ ? "Energy" : "None");
		ImGui::Text("Selected: %s", selectedType);
		ImGui::Text("Charge: %.2f / %.2f sec", lockOnSeconds_, settings_.maxLockOnSeconds);
		const float chargeRatio = CalculateChargeRatio();
		ImGui::Text("Charge Ratio: %.0f%%", chargeRatio * 100.0f);
		ImGui::Text("Requested Energy: %d / %d", CalculateRequestedEnergy(), settings_.maxChargeEnergyCost);

		ImGui::End();
#endif
	}
}
