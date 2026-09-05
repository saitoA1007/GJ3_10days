#include "TitleScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "Application/CollisionConfig.h"
#include "Application/TitleLogo/TitleLogo.h"
#include "Application/Effect/HyperspaceEffect.h"
#include "AudioManager.h"
using namespace GameEngine;

TitleScene::~TitleScene() {}

TitleScene::TitleScene() {

	// 決定ボタンコマンドを追加
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_F } });
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_G } });
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	inputCommand_->RegisterCommand("MoveForward", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveBack", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	// 破壊オブジェクトを元の姿へ戻す
	inputCommand_->RegisterCommand("Reassemble", { {InputState::KeyTrigger, DIK_R } });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	mainCamera_->Update();
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// タイトルロゴを構成するt0.obj～t3.objを生成
	titleLogo_ = std::make_unique<TitleLogo>(modelManager_);

	auto* halfDomeModel = modelManager_->GetNameByModel("halfDome.gltf");
	halfDomeModel->SetDefaultIsEnableLight(false);
	hyperspaceEffect_ = gameObjectManager_->AddObject<HyperspaceEffect>(halfDomeModel);
}

void TitleScene::Initialize() {
	isFinished_ = false;
	titleLogo_->ResetAnimation();
	hyperspaceEffect_->ResetAnimation();
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	// Decision入力を受けたらタイトル終了演出を開始する。
	if (inputCommand_->IsCommandActive("Decision")) {
		if (titleLogo_->GetAnimationState() == AnimationState::Idle) {
			auto& audioManager = AudioManager::GetInstance();
			const uint32_t titleDecisionHandle = audioManager.GetHandleByName("hyperSpace.mp3");
			audioManager.Play(titleDecisionHandle, 1.0f, false);

			titleLogo_->AnimationStart();
			hyperspaceEffect_->StartAnimation();
		}
	}

	// タイトルロゴの更新処理
	titleLogo_->Update();

	// ロゴが画面奥まで移動し終えたらシーン遷移を許可する。
	if (titleLogo_->IsAnimationFinished()) {
		isFinished_ = true;
	}
}

void TitleScene::DebugUpdate() {
	titleLogo_->DebugUpdate();
}

void TitleScene::Draw() {
	titleLogo_->Draw(renderQueue_);
}
