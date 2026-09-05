#include "GameScene.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"
#include <Application/Enemy/EnemyManager.h>
#include "Application/Player/Player.h"
#include "Application/GameCamera/GameCamera.h"
#include "Application/Field/Field.h"
#include "Application//Energy/EnergySpawner.h"
#include "Application/EnergyView/EnergyView.h"
#include "Application/GameFlow/GameFlow.h"
#include "Application//LockOn/LockOnController.h"
#include "Application//Rocket/Rocket.h"
#include "Application//Unit/UnitManager.h"
#include "Application/Field/FieldEffect.h"
#include "Application/Field/ImpactDetectionEffect.h"
#include "Application/Score/ScoreView.h"
#include "ControllerVibration.h"
#include "FPSCounter.h"
#include "Application/Effect/BlackHoleEffect.h"

// 後で別クラスに纏めて消す
namespace
{
	constexpr int kScorePerEnemy = 100;
	constexpr int kChargeVibrationThreshold = 5;        // 振動を開始するためのチャージされたピクミの数
	constexpr float kChargeVibrationLeftMotor = 0.35f;  // 左モーターの振動強度
	constexpr float kChargeVibrationRightMotor = 0.25f; // 右モーターの振動強度
	constexpr Vector3 kCameraPosition = { 0.0f, 60.0f, -60.0f };
	constexpr Vector3 kCameraTarget = { 0.0f, 0.0f, 0.0f };
}

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定s
	InputRegisterCommand();
	controllerVibration_ = std::make_unique<ControllerVibration>(input_);

	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize(
		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, kCameraPosition },
		1280,
		720);


	// 背景を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("rogland_clear_night_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);
	
	// 地面演出
	auto* cubeModel = modelManager_->GetNameByModel("cube.obj");
	cubeModel->SetDefaultIsEnableLight(true);
	auto* fieldEffect = gameObjectManager_->AddObject<FieldEffect>(cubeModel, 0);

	// フィールド
	auto* fieldModel = modelManager_->GetNameByModel("fieldCircle.obj");
	fieldModel->SetDefaultIsEnableLight(true);
	// ポール
	auto* poleModel = modelManager_->GetNameByModel("pole.gltf");
	poleModel->SetDefaultIsEnableLight(false);
	// 円
	auto* circleModel = modelManager_->GetNameByModel("stageCircle.gltf");
	circleModel->SetDefaultIsEnableLight(false);
	// 宇宙を映す平面
	auto* planeXZModel = modelManager_->GetNameByModel("halfDome.gltf");
	planeXZModel->SetDefaultIsEnableLight(false);
	field_ = gameObjectManager_->AddObject<Field>(fieldModel);

	auto* planeModel = modelManager_->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	uint32_t pGH = textureManager_->GetHandleByName("effectCircle.png");
	auto* impactEffect = gameObjectManager_->AddObject<ImpactDetectionEffect>(planeModel, pGH);

	auto* rocketModel = modelManager_->GetNameByModel("rocket.obj");
	rocket_ = gameObjectManager_->AddObject<Rocket>(rocketModel);

	auto* energyModel = modelManager_->GetNameByModel("energy.obj");
	energySpawner_ = gameObjectManager_->AddObject<EnergySpawner>(energyModel, field_);

	auto* unitModel = modelManager_->GetNameByModel("energy.obj");
	unitManager_ = gameObjectManager_->AddObject<UnitManager>(unitModel, rocket_);

	auto* cursorModel = modelManager_->GetNameByModel("cursor.obj");
	lockOnController_ = gameObjectManager_->AddObject<LockOnController>(
		input_,
		inputCommand_,
		mainCamera_.get(),
		cursorModel,
		debugRenderer_,
		field_,
		rocket_,
		energySpawner_,
		//enemyManager_,
		unitManager_);

	// プレイヤーを見下ろしながら追従するメインカメラ
	auto* gameCamera = gameObjectManager_->AddObject<GameCamera>(player_);

	ScoreView::DigitModels digitModels{};
	for (int digit = 0; digit < static_cast<int>(digitModels.size()); ++digit) {
		digitModels[digit] = modelManager_->GetNameByModel(std::to_string(digit) + ".obj");
	}

	// スコア表示
	scoreView_ = std::make_unique<ScoreView>(digitModels, gameCamera->GetCamera());

	//Enemy
	auto enemyModel = modelManager_->GetNameByModel("Enemy.gltf");
	auto* enemies = gameObjectManager_->AddObject<EnemyManager>(128, enemyModel);
	enemies->SetOnEnemyDefeated([this]() {
		score_.Add(kScorePerEnemy);
	});


	gameFlowController_ = gameObjectManager_->AddObject<GameFlowController>(
		rocket_,
		energySpawner_,
		//enemyManager_,
		unitManager_,
		lockOnController_);

	energyView_ = gameObjectManager_->AddObject<EnergyView>(
		digitModels,
		mainCamera_.get(),
		rocket_);
  
  enemies->SetStage("Test");

	// ブラックホールのテスト
	auto* sphereModel = modelManager_->GetNameByModel("sphere.obj");
	auto* ringModel = modelManager_->GetNameByModel("blackHoleRing.gltf");
	gameObjectManager_->AddObject<BlackHoleEffect>(sphereModel, ringModel);
}

void GameScene::Initialize() {
	mainCamera_->transform_.translate = kCameraPosition;
	mainCamera_->transform_.rotate = Math::DirectionToEuler(kCameraTarget - kCameraPosition);
	UpdateCamera();

	score_.Reset();
	scoreView_->SetValue(score_.GetDisplayedValue());
}

void GameScene::Update() {
	score_.Update(FpsCounter::deltaTime);
	scoreView_->SetValue(score_.GetDisplayedValue());
	scoreView_->Update();
	UpdateCamera();
	
	// Playerはゲーム状態だけを公開し、振動の強度と出力はシーン側で管理する。
	if (controllerVibration_ && player_ && player_->GetChargedPikumiCount() >= kChargeVibrationThreshold)
	{
		controllerVibration_->SetVibration(kChargeVibrationLeftMotor, kChargeVibrationRightMotor);
	}
	else if (controllerVibration_)
	{
		controllerVibration_->Stop();
	}
}

void GameScene::DebugUpdate()
{
	scoreView_->SetValue(score_.GetDisplayedValue());
	scoreView_->Update();
	UpdateCamera();
	
	// ゲーム更新を停止している間に振動が残らないようにする。
	if (controllerVibration_)
	{
		controllerVibration_->Stop();
	}
}

void GameScene::Draw() {
	scoreView_->Draw(renderQueue_);
}

void GameScene::InputRegisterCommand() {

	// 決定ボタン
	inputCommand_->RegisterCommand("PauseAction", { {InputState::KeyTrigger, DIK_M},{InputState::PadTrigger, XINPUT_GAMEPAD_START} });
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });
	inputCommand_->RegisterCommand("SelectUp", { {InputState::KeyTrigger, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("SelectDown", { {InputState::KeyTrigger, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_DOWN} });

	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { { InputState::KeyPush, DIK_LEFT },{InputState::PadRightStick,0,{-1.0f,0.0f},0.2f} });
	inputCommand_->RegisterCommand("CameraMoveRight", { { InputState::KeyPush, DIK_RIGHT },{InputState::PadRightStick,0,{1.0f,0.0f},0.2f} });
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { {InputState::KeyTrigger, DIK_L},{InputState::PadTriggerRightTrigger,0,{0.0f,0.0f},0.2f},{InputState::PadTriggerLeftTrigger,0,{0.0f,0.0f},0.2f} });

	inputCommand_->RegisterCommand("CursorUp", {
		{ InputState::KeyPush, DIK_W },
		});
	inputCommand_->RegisterCommand("CursorDown", {
		{ InputState::KeyPush, DIK_S },
		});
	inputCommand_->RegisterCommand("CursorLeft", {
		{ InputState::KeyPush, DIK_A },
		});
	inputCommand_->RegisterCommand("CursorRight", {
		{ InputState::KeyPush, DIK_D },
		});

	inputCommand_->RegisterCommand("LockOnTrigger", {
		{ InputState::MouseTrigger, 0 },
		{ InputState::KeyTrigger, DIK_SPACE },
		{ InputState::PadTrigger, XINPUT_GAMEPAD_A },
		});
	inputCommand_->RegisterCommand("LockOnPush", {
		{ InputState::MousePush, 0 },
		{ InputState::KeyPush, DIK_SPACE },
		{ InputState::PadPush, XINPUT_GAMEPAD_A },
		});
	inputCommand_->RegisterCommand("LockOnRelease", {
		{ InputState::MouseRelease, 0 },
		{ InputState::KeyRelease, DIK_SPACE },
		{ InputState::PadRelease, XINPUT_GAMEPAD_A },
		});
}


void GameScene::UpdateCamera()
{
	mainCamera_->Update();
	renderQueue_->SetCamera(mainCamera_.get());
}
