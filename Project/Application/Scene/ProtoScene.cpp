#include "ProtoScene.h"

#include <string>

#include "Application/Prototype/Enemy/PrototypeEnemyManager.h"
#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/EnergyView/PrototypeEnergyView.h"
#include "Application/Prototype/Field/PrototypeField.h"
#include "Application/Prototype/GameFlow/PrototypeGameFlow.h"
#include "Application/Prototype/LockOn/PrototypeLockOnController.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"
#include "MyMath.h"

using namespace GameEngine;

namespace {
	constexpr Vector3 kCameraPosition = { 0.0f, 60.0f, -60.0f };
	constexpr Vector3 kCameraTarget = { 0.0f, 0.0f, 0.0f };
}

ProtoScene::ProtoScene() {
	RegisterInputCommands();

	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize(
		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, kCameraPosition },
		1280,
		720);

	// 生成順には依存関係がある。FieldとRocketを先に作り、各Managerへ非所有参照を渡す。
	auto* fieldCircleModel = modelManager_->GetNameByModel("fieldCircle.obj");
	field_ = gameObjectManager_->AddObject<Prototype::Field>(fieldCircleModel);

	auto* rocketModel = modelManager_->GetNameByModel("rocket.obj");
	rocket_ = gameObjectManager_->AddObject<Prototype::Rocket>(rocketModel);

	auto* energyModel = modelManager_->GetNameByModel("energy.obj");
	energySpawner_ = gameObjectManager_->AddObject<Prototype::EnergySpawner>(energyModel, field_);

	auto* unitModel = modelManager_->GetNameByModel("unit.obj");
	unitManager_ = gameObjectManager_->AddObject<Prototype::UnitManager>(unitModel, rocket_);

	auto* enemyModel = modelManager_->GetNameByModel("enemy.obj");
	enemyManager_ = gameObjectManager_->AddObject<Prototype::EnemyManager>(
		enemyModel,
		field_,
		rocket_,
		energySpawner_,
		unitManager_);

	auto* cursorModel = modelManager_->GetNameByModel("cursor.obj");
	lockOnController_ = gameObjectManager_->AddObject<Prototype::LockOnController>(
		input_,
		inputCommand_,
		mainCamera_.get(),
		cursorModel,
		debugRenderer_,
		field_,
		rocket_,
		energySpawner_,
		enemyManager_,
		unitManager_);

	gameFlowController_ = gameObjectManager_->AddObject<Prototype::GameFlowController>(
		rocket_,
		energySpawner_,
		enemyManager_,
		unitManager_,
		lockOnController_);

	// ScoreViewを再利用するため、0.obj～9.objを数字順に配列へ格納する。
	ScoreView::DigitModels digitModels{};
	for (int digit = 0; digit < static_cast<int>(digitModels.size()); ++digit) {
		digitModels[digit] = modelManager_->GetNameByModel(std::to_string(digit) + ".obj");
	}
	energyView_ = gameObjectManager_->AddObject<Prototype::EnergyView>(
		digitModels,
		mainCamera_.get(),
		rocket_);
}

void ProtoScene::Initialize() {
	isFinished_ = false;

	mainCamera_->transform_.translate = kCameraPosition;
	mainCamera_->transform_.rotate = Math::DirectionToEuler(kCameraTarget - kCameraPosition);
	UpdateCamera();

	const uint32_t skyboxHandle = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxHandle);
}

void ProtoScene::Update() {
	UpdateCamera();
}

void ProtoScene::DebugUpdate() {
	UpdateCamera();
	DrawOriginGuide();
}

void ProtoScene::Draw() {
	// StaticGameObjectManager がエディタから追加したオブジェクトを描画する。
}

void ProtoScene::RegisterInputCommands() {
	// デバイス差をここで吸収し、LockOnControllerはコマンド名だけを参照する。
	inputCommand_->RegisterCommand("ProtoCursorUp", {
		{ InputState::KeyPush, DIK_W },
	});
	inputCommand_->RegisterCommand("ProtoCursorDown", {
		{ InputState::KeyPush, DIK_S },
	});
	inputCommand_->RegisterCommand("ProtoCursorLeft", {
		{ InputState::KeyPush, DIK_A },
	});
	inputCommand_->RegisterCommand("ProtoCursorRight", {
		{ InputState::KeyPush, DIK_D },
	});

	inputCommand_->RegisterCommand("ProtoLockOnTrigger", {
		{ InputState::MouseTrigger, 0 },
		{ InputState::KeyTrigger, DIK_SPACE },
		{ InputState::PadTrigger, XINPUT_GAMEPAD_A },
	});
	inputCommand_->RegisterCommand("ProtoLockOnPush", {
		{ InputState::MousePush, 0 },
		{ InputState::KeyPush, DIK_SPACE },
		{ InputState::PadPush, XINPUT_GAMEPAD_A },
	});
	inputCommand_->RegisterCommand("ProtoLockOnRelease", {
		{ InputState::MouseRelease, 0 },
		{ InputState::KeyRelease, DIK_SPACE },
		{ InputState::PadRelease, XINPUT_GAMEPAD_A },
	});
}

void ProtoScene::UpdateCamera() {
	mainCamera_->Update();
	renderQueue_->SetCamera(mainCamera_.get());
}

void ProtoScene::DrawOriginGuide() {
	// 停止中でも原点と各軸を見失わないためのガイド。
	debugRenderer_->AddLine({ 0.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f }, { 1.0f, 0.2f, 0.2f, 1.0f });
	debugRenderer_->AddLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.2f, 1.0f, 0.2f, 1.0f });
	debugRenderer_->AddLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 5.0f }, { 0.2f, 0.4f, 1.0f, 1.0f });
}
