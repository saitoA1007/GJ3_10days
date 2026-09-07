#include "TestScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "Application/CollisionConfig.h"
#include "Application/Field/ImpactDetectionEffect.h"
#include "Application/Effect/EnemySpawnEffect.h"
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {

    // 決定ボタンコマンドを追加
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_F }});
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_G }});
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

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("walk.gltf");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {2.0f,2.0f,2.0f},{0.0f,0.0f,0.0f},{2.0f,-1.0f,0.0f} });

	// アニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// アニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

	// 地面
	terrainModel_ = modelManager_->GetNameByModel("terrain.obj");
	terrainModel_->SetDefaultIsEnableLight(true);
	terrainModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	terrainModel_->SetDefaultTextureHandle(grassGH);
	terrainWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-4.0f,0.0f} });

	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	terrainModel_->SetDefaultNormalTexture(normalGH);

	// エフェクト用モデル
	effectModel_ = modelManager_->GetNameByModel("plane.obj");
	effectModel_->SetDefaultIsEnableLight(false);
	primitiveEffect_ = gameObjectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 128, textureManager_, effectModel_);

	//gameObjectManager_->AddObject<ParticleBehavior>("EnergyEffect", 16, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("RocketFireEffect", 32, textureManager_, effectModel_);

	auto* sModel = modelManager_->GetNameByModel("beam.gltf");
	sModel->SetDefaultIsEnableLight(false);
	auto* rModel = modelManager_->GetNameByModel("RushPower.obj");
	rModel->SetDefaultIsEnableLight(false);
	gameObjectManager_->AddObject<EnemySpawnEffect>(sModel,effectModel_, rModel, textureManager_);

	//uint32_t pGH = textureManager_->GetHandleByName("effectCircle.png");
	//gameObjectManager_->AddObject<ImpactDetectionEffect>(effectModel_, pGH);

	auto* cModel = modelManager_->GetNameByModel("Crystal.gltf");
	cModel->SetDefaultIsEnableLight(false);
	m_ = std::make_unique<ModelComponent>(cModel);
	m_->SetHitGroup(6);
	m_->SetBufferMaterial(0, mat_.GetMaterialSrvIndex());

	gameObjectManager_->AddObject<ParticleBehavior>("EnemyCommonEffect", 16, textureManager_, effectModel_);
	auto pModel = modelManager_->GetNameByModel("Prick.gltf");
	gameObjectManager_->AddObject<ParticleBehavior>("EnemyDeadEffect", 256, textureManager_, pModel);
}

void TestScene::Initialize() {

}

void TestScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	world_.UpdateTransformMatrix();

	// アニメーションの更新処理
	walkAnimator_->ComputeUpdate();

	m_->Update();

	mat_.materialData_->time += FpsCounter::gameDeltaTime;

	DebugUpdate();
}

void TestScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::DragFloat3("PlayerPos", &m_->worldTransform_.transform_.translate.x, 0.1f);
	ImGui::DragFloat3("PlayerScale", &m_->worldTransform_.transform_.scale.x, 0.1f);
	ImGui::ColorEdit4("PlayerColor", &playerColor_.x);

	// 
	primitiveEffect_->SetEmitterPos(m_->worldTransform_.transform_.translate);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	world_.UpdateTransformMatrix();
	model_->SetDefaultColor(playerColor_);

	if (ImGui::CollapsingHeader("Crystal Material", ImGuiTreeNodeFlags_DefaultOpen)) {

		// 1. カラー設定
		if (ImGui::TreeNode("Color Settings")) {
			ImGui::ColorEdit4("Base Color", &mat_.materialData_->baseColor.x);
			ImGui::ColorEdit4("Rim Color", &mat_.materialData_->rimColor.x);
			ImGui::ColorEdit4("Dissolve Edge Color", &mat_.materialData_->dissolveEdgeColor.x);
			ImGui::TreePop();
		}

		// 2. ディゾルブ (消滅) エフェクト
		if (ImGui::TreeNode("Dissolve Settings")) {
			ImGui::SliderFloat("Threshold", &mat_.materialData_->dissolveThreshold, 0.0f, 1.0f);
			ImGui::DragFloat("Edge Width", &mat_.materialData_->dissolveEdgeWidth, 0.005f, 0.0f, 1.0f);
			ImGui::DragFloat("Noise Scale", &mat_.materialData_->dissolveNoiseScale, 0.1f, 0.0f, 100.0f);
			ImGui::InputScalar("Dissolve Tex Handle", ImGuiDataType_U32, &mat_.materialData_->dissolveTextureHandle);
			ImGui::TreePop();
		}

		// 3. リムライト
		if (ImGui::TreeNode("Rim Light Settings")) {
			ImGui::DragFloat("Rim Power", &mat_.materialData_->rimPower, 0.1f, 0.0f, 50.0f);
			ImGui::DragFloat("Rim Intensity", &mat_.materialData_->rimIntensity, 0.05f, 0.0f, 10.0f);
			ImGui::TreePop();
		}

		// 4. 屈折・宇宙エフェクト
		if (ImGui::TreeNode("Refraction & Universe")) {
			ImGui::SliderFloat("IOR (Index of Refraction)", &mat_.materialData_->ior, 1.0f, 3.0f);
			ImGui::SliderFloat("Fresnel Strength", &mat_.materialData_->fresnelStrength, 0.0f, 1.0f);
			ImGui::DragFloat("Universe Intensity", &mat_.materialData_->universeIntensity, 0.05f, 0.0f, 10.0f);
			ImGui::DragFloat("Universe Scale", &mat_.materialData_->universeScale, 0.1f, 0.0f, 100.0f);
			ImGui::TreePop();
		}

		// 6. システム・その他パラメータ
		if (ImGui::TreeNode("System / Textures")) {
			ImGui::DragFloat("Time", &mat_.materialData_->time, 0.01f);
			ImGui::InputScalar("Albedo Tex Handle", ImGuiDataType_U32, &mat_.materialData_->textureHandle);
			ImGui::TreePop();
		}
	}
	ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	//m_->DrawCustomRaytracing(renderQueue_);
}
