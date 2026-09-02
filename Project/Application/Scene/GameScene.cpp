#include "GameScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"
#include <Application/Enemy/EnemyManager.h>
#include "Application/Player/Player.h"
#include "Application/GameCamera/GameCamera.h"
#include "Application/Field/Field.h"
#include "Application/Field/ImpactDetectionEffect.h"
#include "Application/Tower/Tower.h"
#include "ControllerVibration.h"

// 後で別クラスに纏めて消す
namespace
{
	constexpr int kChargeVibrationThreshold = 5;        // 振動を開始するためのチャージされたピクミの数
	constexpr float kChargeVibrationLeftMotor = 0.35f;  // 左モーターの振動強度
	constexpr float kChargeVibrationRightMotor = 0.25f; // 右モーターの振動強度
}

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定s
	InputRegisterCommand();
	controllerVibration_ = std::make_unique<ControllerVibration>(input_);

	// 背景を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("qwantani_moon_noon_puresky_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// フィールド
	auto* fieldModel = modelManager_->GetNameByModel("cylinder.gltf");
	fieldModel->SetDefaultIsEnableLight(true);
	auto field = gameObjectManager_->AddObject<Field>(fieldModel);

	auto* planeModel = modelManager_->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	uint32_t pGH = textureManager_->GetHandleByName("effectCircle.png");
	auto* impactEffect = gameObjectManager_->AddObject<ImpactDetectionEffect>(planeModel, pGH);

	// タワー
	auto* towerModel = modelManager_->GetNameByModel("cube.obj");
	towerModel->SetDefaultIsEnableLight(true);
	auto tower = gameObjectManager_->AddObject<Tower>(towerModel);

	// プレイヤー
	auto* playerModel = modelManager_->GetNameByModel("cube.obj");
	auto* pikumiModel = modelManager_->GetNameByModel("sphere.obj");
	playerModel->SetDefaultIsEnableLight(true);
	pikumiModel->SetDefaultIsEnableLight(true);
	auto player = gameObjectManager_->AddObject<Player>(inputCommand_, playerModel, pikumiModel, field, impactEffect);

	// プレイヤーを見下ろしながら追従するメインカメラ
	gameObjectManager_->AddObject<GameCamera>(player_);

	//Enemy
	auto enemyModel = modelManager_->GetNameByModel("Enemy.obj");
	gameObjectManager_->AddObject<EnemyManager>(32, enemyModel);
}

void GameScene::Initialize() {

	
}

void GameScene::Update() {
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
	// ゲーム更新を停止している間に振動が残らないようにする。
	if (controllerVibration_)
	{
		controllerVibration_->Stop();
	}
}

void GameScene::Draw() {
	
}

void GameScene::InputRegisterCommand() {

	// 決定ボタン
	inputCommand_->RegisterCommand("PauseAction", { {InputState::KeyTrigger, DIK_M},{InputState::PadTrigger, XINPUT_GAMEPAD_START} });
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });
	inputCommand_->RegisterCommand("SelectUp", { {InputState::KeyTrigger, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("SelectDown", { {InputState::KeyTrigger, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_DOWN} });

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_W }, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_S }, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A }, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D }, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ピクミ発射コマンドを登録する
	inputCommand_->RegisterCommand("Shot", { {InputState::KeyPush, DIK_SPACE},{InputState::PadPush, XINPUT_GAMEPAD_A} });

	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { { InputState::KeyPush, DIK_LEFT },{InputState::PadRightStick,0,{-1.0f,0.0f},0.2f} });
	inputCommand_->RegisterCommand("CameraMoveRight", { { InputState::KeyPush, DIK_RIGHT },{InputState::PadRightStick,0,{1.0f,0.0f},0.2f} });
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { {InputState::KeyTrigger, DIK_L},{InputState::PadTriggerRightTrigger,0,{0.0f,0.0f},0.2f},{InputState::PadTriggerLeftTrigger,0,{0.0f,0.0f},0.2f} });

	// AttackDownコマンド
	inputCommand_->RegisterCommand("AttackDown", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushChargeコマンド
	inputCommand_->RegisterCommand("RushCharge", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushStartコマンド
	inputCommand_->RegisterCommand("RushStart", { {InputState::MouseRelease, 1}, {InputState::PadRelease, XINPUT_GAMEPAD_X} });
}
