#include "PrototypeScene.h"
#include "Application/GameObject/Player/Player.h"

using namespace GameEngine;

PrototypeScene::PrototypeScene() {
	InputRegisterCommand();
}

void PrototypeScene::Initialize() {

	// メインカメラの初期化
	mainCamera_ = std::make_unique<GameEngine::Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	mainCamera_->Update();

	// プロトタイプ用プレイヤーの初期化
	auto* playerModel = modelManager_->GetNameByModel("cube.obj");
	player_ = gameObjectManager_->AddObject<Player>(inputCommand_, playerModel);
}

void PrototypeScene::Update() {
	// カメラの更新処理
	mainCamera_->Update();

	player_->Update();

	DebugUpdate();
}

void PrototypeScene::DebugUpdate() {
}

void PrototypeScene::Draw() {
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());
}

void PrototypeScene::InputRegisterCommand() {
	inputCommand_->RegisterCommand("MoveUp", { { InputState::KeyPush, DIK_W } });
	inputCommand_->RegisterCommand("MoveDown", { { InputState::KeyPush, DIK_S } });
	inputCommand_->RegisterCommand("MoveLeft", { { InputState::KeyPush, DIK_A } });
	inputCommand_->RegisterCommand("MoveRight", { { InputState::KeyPush, DIK_D } });
}
