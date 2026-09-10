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

namespace {
	constexpr Vector3 kCameraPosition = { 0.0f, 1.0f, -15.0f };
	constexpr float kStartDelayDuration = 0.0f;
}

TitleScene::~TitleScene() {}

TitleScene::TitleScene() {

	// 決定ボタンコマンドを追加
	inputCommand_->RegisterCommand("Decision", {
		{ InputState::KeyTrigger, DIK_SPACE },
		{ InputState::MouseTrigger, 0 },
		});
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
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},kCameraPosition }, 1280, 720);
	mainCamera_->Update();
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// タイトルロゴを構成するt0.obj～t3.objを生成
	titleLogo_ = std::make_unique<TitleLogo>(modelManager_, input_->IsPadConnected());

	auto* halfDomeModel = modelManager_->GetNameByModel("halfDome.gltf");
	halfDomeModel->SetDefaultIsEnableLight(false);
	hyperspaceEffect_ = gameObjectManager_->AddObject<HyperspaceEffect>(halfDomeModel);

	// 安全策
	Dissolve* dissolve = postEffectManager_->GetPostEffect<Dissolve>("DissolvePass");
	dissolve->SetThreshold(0.0f);
}

void TitleScene::Initialize() {
	isFinished_ = false;
	startDelayTimer_.Start(kStartDelayDuration);
	titleLogo_->ResetAnimation();
	hyperspaceEffect_->ResetAnimation();
	hyperspaceEffect_->SetActive(false);
	mainCamera_->transform_.translate = kCameraPosition;
	mainCamera_->Update();
}

void TitleScene::Update() {
	// 10秒経過するまでは、タイトルシーン内の演出・入力・音声を開始しない。
	startDelayTimer_.Update(FpsCounter::deltaTime);
	if (!startDelayTimer_.IsFinished()) {
		return;
	}
	hyperspaceEffect_->SetActive(true);

	// Decision入力を受けたらタイトル終了演出を開始する。
	if (inputCommand_->IsCommandActive("Decision")) {
		if (titleLogo_->GetAnimationState() == AnimationState::Idle) {
			auto& audioManager = AudioManager::GetInstance();
			const uint32_t titleDecisionHandle = audioManager.GetHandleByName("titleDecision.mp3");
			audioManager.Play(titleDecisionHandle, 1.0f, false);

			titleLogo_->AnimationStart();
			hyperspaceEffect_->StartAnimation();
			if (!hyperspaceAudioTimer_.IsActive()) {
				hyperspaceAudioTimer_.Start(0.25f);
			}

			if (!bgmAudioTimer_.IsActive()) {
				bgmAudioTimer_.Start(1.0f);
			}
		}
	}

	// タイトルロゴの更新処理
	titleLogo_->Update();

	// タイトル終了演出中は、ロゴのシェイクと同期してカメラも揺らす。
	// 毎フレーム基準位置からオフセットすることで、位置のずれが累積しないようにする。
	mainCamera_->transform_.translate = kCameraPosition + titleLogo_->GetCameraShakeOffset();
	mainCamera_->Update();

	hyperspaceAudioTimer_.Update();
	bgmAudioTimer_.Update();

	{
		if(bgmAudioTimer_.IsActive()) {
			auto& audioManager = AudioManager::GetInstance();
			const uint32_t titleBGM = audioManager.GetHandleByName("titleBGM.mp3");
			audioManager.SetVolume(titleBGM, 1.0f - bgmAudioTimer_.GetProgress());
		}
	}

	if (hyperspaceAudioTimer_.IsFinished() && !isHyperspaceAudioPlayed_) {
		auto& audioManager = AudioManager::GetInstance();
		const uint32_t titleDecisionHandle = audioManager.GetHandleByName("titleHyperSpace.mp3");
		audioManager.Play(titleDecisionHandle, 1.0f, false);
		isHyperspaceAudioPlayed_ = true;
	}

	// ロゴが画面奥まで移動し終えたらシーン遷移を許可する。
	if (titleLogo_->IsAnimationFinished()) {
		isFinished_ = true;
		auto& audioManager = AudioManager::GetInstance();
		const uint32_t titleBGM = audioManager.GetHandleByName("titleBGM.mp3");
		audioManager.Stop(titleBGM);
	}
}

void TitleScene::DebugUpdate() {
	titleLogo_->DebugUpdate();
}

void TitleScene::Draw() {
	if (!startDelayTimer_.IsFinished()) {
		return;
	}

	titleLogo_->Draw(renderQueue_);
}
