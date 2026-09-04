#include "ProtoScene.h"

#include "Application/Prototype/Field/PrototypeField.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "MyMath.h"

using namespace GameEngine;

namespace {
	constexpr Vector3 kCameraPosition = { 0.0f, 60.0f, -60.0f };
	constexpr Vector3 kCameraTarget = { 0.0f, 0.0f, 0.0f };
}

ProtoScene::ProtoScene() {
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize(
		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, kCameraPosition },
		1280,
		720);

	auto* fieldCircleModel = modelManager_->GetNameByModel("fieldCircle.obj");
	field_ = gameObjectManager_->AddObject<Prototype::Field>(fieldCircleModel);

	auto* rocketModel = modelManager_->GetNameByModel("rocket.obj");
	rocket_ = gameObjectManager_->AddObject<Prototype::Rocket>(rocketModel);
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
