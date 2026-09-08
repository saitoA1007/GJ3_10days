#include "GameScene.h"
using namespace GameEngine;
#include "ImguiManager.h"
#include "ParticleBehavior.h"
#include "PostProcess/PostEffectData.h"
#include <Application/Enemy/EnemyManager.h>
#include "Application/Enemy/EnemyEffectManager.h"
#include "Application/Player/Player.h"
#include "Application/GameCamera/GameCamera.h"
#include "Application/Field/Field.h"
#include "Application//Energy/EnergySpawner.h"
#include "Application/EnergyView/EnergyView.h"
#include "Application/GameFlow/GameFlow.h"
#include "Application/GameFlow/TutorialPhase.h"
#include "Application//LockOn/LockOnController.h"
#include "Application//Rocket/Rocket.h"
#include "Application//Unit/UnitManager.h"
#include "Application/Field/FieldEffect.h"
#include "Application/Score/ScoreView.h"
#include "Application/StartPlaying/StartPlayingView.h"
#include "Application/Tutorial/TutorialCameraModelView.h"
#include "Application/Tutorial/TutorialTextSequence.h"
#include "ControllerVibration.h"
#include "DebugParameter.h"
#include "FPSCounter.h"
#include "Application/Effect/BlackHoleEffect.h"
#include "Application/Effect/SpawnFieldEffect.h"
#include "Application/Effect/MoonObject.h"
#include "Application/Effect/RocketEffect.h"
#include "Application/Effect/ExplosionEffect.h"
#include <Application/result/ShuffleNumber.h>
#include "Application/result/ResultMovieManager.h"
#include "Application/GameCamera/ResultMoveCamera.h"
#include <algorithm>
#include <string>
#include <string_view>
#include "MyMath.h"

#include "Application/UI/TimeUI.h"

// 後で別クラスに纏めて消す
namespace
{
	constexpr int kScorePerEnemy = 100;
	constexpr int kChargeVibrationThreshold = 5;        // 振動を開始するためのチャージされたピクミの数
	constexpr float kChargeVibrationLeftMotor = 0.35f;  // 左モーターの振動強度
	constexpr float kChargeVibrationRightMotor = 0.25f; // 右モーターの振動強度
	constexpr Vector3 kCameraPosition = { 0.0f, 60.0f, -60.0f };
	constexpr Vector3 kCameraTarget = { 0.0f, 0.0f, 0.0f };
	constexpr float kFadeDuration = 1.0f;
	constexpr Vector2 kFadeTextureSize = { 128.0f, 72.0f };
	constexpr Vector2 kFadeScale = { 10.0f, 10.0f };
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
	mainCameraEndRotation_ = mainCamera_->transform_.rotate;
	mainCameraDebugParameter_ = std::make_unique<DebugParameter>("GameSceneMainCamera");
	mainCameraDebugParameter_->Register("Translate", mainCamera_->transform_.translate, 0);
	mainCameraDebugParameter_->Register("Rotate", mainCameraEndRotation_, 1);
	mainCameraDebugParameter_->Register("StartRotateX", mainCameraEntranceStartRotateX_, 0, "Entrance");

	dir_.Normalize();

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
	field_ = gameObjectManager_->AddObject<Field>(fieldModel);

	// 敵の演出管理機能
	auto* enemyEffectManager = gameObjectManager_->AddObject<EnemyEffectManager>(modelManager_, textureManager_, gameObjectManager_);
	//Enemy
	enemyManager_ = gameObjectManager_->AddObject<EnemyManager>(128, enemyEffectManager, textureManager_, modelManager_);
	enemyManager_->SetOnEnemyDefeated([this]() {
		score_.Add(kScorePerEnemy);
		});

	auto* planeModel = modelManager_->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	//uint32_t pGH = textureManager_->GetHandleByName("effectCircle.png");
	//auto* impactEffect = gameObjectManager_->AddObject<ImpactDetectionEffect>(planeModel, pGH);

	auto* rocketModel = modelManager_->GetNameByModel("Rocket.gltf");
	rocket_ = gameObjectManager_->AddObject<Rocket>(rocketModel);

	auto* energyModel = modelManager_->GetNameByModel("Crystal.gltf");
	energySpawner_ = gameObjectManager_->AddObject<EnergySpawner>(energyModel, field_,textureManager_, planeModel);

	auto* crossBeamModel = modelManager_->GetNameByModel("crossBeam.gltf");
	crossBeamModel->SetDefaultIsEnableLight(false);
	uint32_t beamNoiseGH = textureManager_->GetHandleByName("beamNoise.png");
	auto* unitModel = modelManager_->GetNameByModel("energy.obj");
	unitManager_ = gameObjectManager_->AddObject<UnitManager>(unitModel, rocket_, crossBeamModel, beamNoiseGH);

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
		enemyManager_,
		unitManager_);

	// プレイヤーを見下ろしながら追従するメインカメラ
	auto* gameCamera = gameObjectManager_->AddObject<GameCamera>(player_);

	// Scoreと同様にカメラへ追従するチュートリアル表示
	auto* tutorialLogoModel = modelManager_->GetNameByModel("tutorialLogo.obj");
	tutorialLogoView_ = std::make_unique<TutorialCameraModelView>(
		tutorialLogoModel,
		gameCamera->GetCamera(),
		"TutorialLogo",
		TutorialCameraModelView::Settings{});

	StartPlayingView::Models startPlayingModels{};
	for (std::size_t i = 0; i < startPlayingModels.size(); ++i)
	{
		startPlayingModels[i] = modelManager_->GetNameByModel(
			"playStartLogo" + std::to_string(i) + ".obj");
	}
	startPlayingView_ = std::make_unique<StartPlayingView>(
		startPlayingModels,
		gameCamera->GetCamera());

	// クリアのムービー
	// 月のオブジェクト
	auto* sphereModel = modelManager_->GetNameByModel("moon.gltf");
	auto* fructureModel = modelManager_->GetNameByModel("fractureMoon.gltf");
	uint32_t moonGH = textureManager_->GetHandleByName("moon_meteor_01_diff_1k.jpg");
	uint32_t moonNorGH = textureManager_->GetHandleByName("moon_meteor_01_nor_gl_1k.png");
	auto* moonObject = gameObjectManager_->AddObject<MoonObject>(sphereModel, fructureModel, moonGH, moonNorGH);
	// リザルトのムービーカメラ
	auto* reCamera =  gameObjectManager_->AddObject<ResultMoveCamera>();
	// ロケット演出
	auto* rocketEffect =  gameObjectManager_->AddObject<RocketEffect>(modelManager_, textureManager_,gameObjectManager_);
	// 爆破演出
	auto* explosionEffect = gameObjectManager_->AddObject<ExplosionEffect>(modelManager_, textureManager_, gameObjectManager_);
	// リザルトムービー管理
	auto* resultMoiveManager = gameObjectManager_->AddObject<ResultMovieManager>(reCamera, moonObject, rocketEffect, explosionEffect);
	// リザルトメッセージ
	auto resultMessage = gameObjectManager_->AddObject<ResultMessage>(modelManager_, animationManager_);

	ScoreView::DigitModels digitModels{};
	for (int digit = 0; digit < static_cast<int>(digitModels.size()); ++digit) {
		digitModels[digit] = modelManager_->GetNameByModel(std::to_string(digit) + ".obj");
	}

	// スコア表示
	scoreView_ = std::make_unique<ScoreView>(digitModels, gameCamera->GetCamera());

	enemyManager_->SetContext(
		field_,
		rocket_,
		energySpawner_,
		unitManager_
	);
	enemyManager_->SetStage("Tutorial");

	GameFlowContext flowContext{};
	flowContext.rocket = rocket_;
	flowContext.energySpawner = energySpawner_;
	flowContext.enemyManager = enemyManager_;
	flowContext.unitManager = unitManager_;
	flowContext.lockOnController = lockOnController_;
	flowContext.inputCommand = inputCommand_;
	flowContext.resultMovieManager_ = resultMoiveManager;
	flowContext.tutorialLogoView = tutorialLogoView_.get();
	flowContext.startPlayingView = startPlayingView_.get();
	flowContext.resultMessage = resultMessage;

	gameFlow_ = gameObjectManager_->AddObject<GameFlow>(flowContext);

	// TutorialPhaseの工程に対応するTextを、登録順に表示する。
	TutorialTextSequence::StepDefinition text0Definition{};
	text0Definition.phaseStep = TutorialPhase::Step::SelectEnergy;
	text0Definition.model = modelManager_->GetNameByModel("tutorialText0.obj");
	text0Definition.parameterGroupName = "TutorialText0";
	text0Definition.viewSettings.startPosition = { 16.0f, -29.0f, -26.0f };
	text0Definition.viewSettings.endPosition = { 5.5f, -29.0f, -26.0f };
	text0Definition.viewSettings.rotation = { 2.01099992f, 3.14159274f, 0.0f };
	text0Definition.viewSettings.scale = 0.5f;
	text0Definition.viewSettings.moveDuration = 0.5f;
	text0Definition.viewSettings.easeType = EaseType::kEaseOutElastic;

	TutorialTextSequence::StepDefinition text1Definition{};
	text1Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text1Definition.model = modelManager_->GetNameByModel("tutorialText1.obj");
	text1Definition.parameterGroupName = "TutorialText1";
	text1Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text1Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text1Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text1Definition.viewSettings.scale = 1.0f;
	text1Definition.viewSettings.startDelay = 0.5f;
	text1Definition.viewSettings.moveDuration = 0.5f;
	text1Definition.viewSettings.holdDuration = 2.5f;
	text1Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text1Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text2Definition{};
	text2Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text2Definition.model = modelManager_->GetNameByModel("tutorialText2.obj");
	text2Definition.parameterGroupName = "TutorialText2";
	text2Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text2Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text2Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text2Definition.viewSettings.scale = 1.0f;
	text2Definition.viewSettings.startDelay = 0.5f;
	text2Definition.viewSettings.moveDuration = 0.5f;
	text2Definition.viewSettings.holdDuration = 3.5f;
	text2Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text2Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text3Definition{};
	text3Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text3Definition.model = modelManager_->GetNameByModel("tutorialText3.obj");
	text3Definition.parameterGroupName = "TutorialText3";
	text3Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text3Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text3Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text3Definition.viewSettings.scale = 1.0f;
	text3Definition.viewSettings.startDelay = 0.5f;
	text3Definition.viewSettings.moveDuration = 0.5f;
	text3Definition.viewSettings.holdDuration = 3.5f;
	text3Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text3Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text4Definition{};
	text4Definition.phaseStep = TutorialPhase::Step::ChargeEnergy;
	text4Definition.model = modelManager_->GetNameByModel("tutorialText4.obj");
	text4Definition.parameterGroupName = "TutorialText4";
	text4Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text4Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text4Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text4Definition.viewSettings.scale = 1.0f;
	text4Definition.viewSettings.startDelay = 0.5f;
	text4Definition.viewSettings.moveDuration = 0.5f;
	text4Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text4Definition.onActivated = [this]()
	{
		if (gameFlow_)
		{
			gameFlow_->BeginTutorialChargeEnergyStep();
		}
	};

	TutorialTextSequence::StepDefinition text5Definition{};
	text5Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text5Definition.model = modelManager_->GetNameByModel("tutorialText5.obj");
	text5Definition.parameterGroupName = "TutorialText5";
	text5Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text5Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text5Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text5Definition.viewSettings.scale = 1.0f;
	text5Definition.viewSettings.startDelay = 0.5f;
	text5Definition.viewSettings.moveDuration = 0.5f;
	text5Definition.viewSettings.holdDuration = 3.5f;
	text5Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text5Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text6Definition{};
	text6Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text6Definition.model = modelManager_->GetNameByModel("tutorialText6.obj");
	text6Definition.parameterGroupName = "TutorialText6";
	text6Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text6Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text6Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text6Definition.viewSettings.scale = 1.0f;
	text6Definition.viewSettings.startDelay = 0.5f;
	text6Definition.viewSettings.moveDuration = 0.5f;
	text6Definition.viewSettings.holdDuration = 3.5f;
	text6Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text6Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text7Definition{};
	text7Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text7Definition.model = modelManager_->GetNameByModel("tutorialText7.obj");
	text7Definition.parameterGroupName = "TutorialText7";
	text7Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text7Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text7Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text7Definition.viewSettings.scale = 1.0f;
	text7Definition.viewSettings.startDelay = 0.5f;
	text7Definition.viewSettings.moveDuration = 0.5f;
	text7Definition.viewSettings.holdDuration = 3.5f;
	text7Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text7Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text8Definition{};
	text8Definition.phaseStep = TutorialPhase::Step::EnemyCollision;
	text8Definition.model = modelManager_->GetNameByModel("tutorialText8.obj");
	text8Definition.parameterGroupName = "TutorialText8";
	text8Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text8Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text8Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text8Definition.viewSettings.scale = 1.0f;
	text8Definition.viewSettings.startDelay = 0.5f;
	text8Definition.viewSettings.moveDuration = 0.5f;
	text8Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text8Definition.onActivated = [this]()
	{
		if (gameFlow_)
		{
			gameFlow_->BeginTutorialEnemyCollisionStep();
		}
	};

	TutorialTextSequence::StepDefinition text9Definition{};
	text9Definition.phaseStep = TutorialPhase::Step::EnemyLockOnOrCollision;
	text9Definition.model = modelManager_->GetNameByModel("tutorialText9.obj");
	text9Definition.parameterGroupName = "TutorialText9";
	text9Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text9Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text9Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text9Definition.viewSettings.scale = 1.0f;
	text9Definition.viewSettings.startDelay = 0.5f;
	text9Definition.viewSettings.moveDuration = 0.5f;
	text9Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text9Definition.onActivated = [this]()
	{
		if (gameFlow_)
		{
			gameFlow_->BeginTutorialEnemyLockOnStep();
		}
	};

	TutorialTextSequence::StepDefinition text10Definition{};
	text10Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text10Definition.model = modelManager_->GetNameByModel("tutorialText10.obj");
	text10Definition.parameterGroupName = "TutorialText10";
	text10Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text10Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text10Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text10Definition.viewSettings.scale = 1.0f;
	text10Definition.viewSettings.startDelay = 0.5f;
	text10Definition.viewSettings.moveDuration = 0.5f;
	text10Definition.viewSettings.holdDuration = 3.5f;
	text10Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text10Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text11Definition{};
	text11Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text11Definition.model = modelManager_->GetNameByModel("tutorialText11.obj");
	text11Definition.parameterGroupName = "TutorialText11";
	text11Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text11Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text11Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text11Definition.viewSettings.scale = 1.0f;
	text11Definition.viewSettings.startDelay = 0.5f;
	text11Definition.viewSettings.moveDuration = 0.5f;
	text11Definition.viewSettings.holdDuration = 3.5f;
	text11Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text11Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition text12Definition{};
	text12Definition.phaseStep = TutorialPhase::Step::EnemyHoldOrCollision;
	text12Definition.model = modelManager_->GetNameByModel("tutorialText12.obj");
	text12Definition.parameterGroupName = "TutorialText12";
	text12Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text12Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text12Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text12Definition.viewSettings.scale = 1.0f;
	text12Definition.viewSettings.startDelay = 0.5f;
	text12Definition.viewSettings.moveDuration = 0.5f;
	text12Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text12Definition.onActivated = [this]()
	{
		if (gameFlow_)
		{
			gameFlow_->BeginTutorialEnemyHoldStep();
		}
	};

	TutorialTextSequence::StepDefinition text13Definition{};
	text13Definition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	text13Definition.model = modelManager_->GetNameByModel("tutorialText13.obj");
	text13Definition.parameterGroupName = "TutorialText13";
	text13Definition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	text13Definition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	text13Definition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	text13Definition.viewSettings.scale = 1.0f;
	text13Definition.viewSettings.startDelay = 0.5f;
	text13Definition.viewSettings.moveDuration = 0.5f;
	text13Definition.viewSettings.holdDuration = 3.5f;
	text13Definition.viewSettings.easeType = EaseType::kEaseOutExpo;
	text13Definition.showSuccessColor = false;

	TutorialTextSequence::StepDefinition textEndDefinition{};
	textEndDefinition.progressMode = TutorialTextSequence::ProgressMode::TimedHold;
	textEndDefinition.model = modelManager_->GetNameByModel("tutorialTextEnd.obj");
	textEndDefinition.parameterGroupName = "TutorialTextEnd";
	textEndDefinition.viewSettings.startPosition = { 0.0f, -40.0f, -25.0f };
	textEndDefinition.viewSettings.endPosition = { 0.0f, -34.5f, -25.0f };
	textEndDefinition.viewSettings.rotation = { 1.721f, 3.14159274f, 0.0f };
	textEndDefinition.viewSettings.scale = 1.0f;
	textEndDefinition.viewSettings.startDelay = 0.5f;
	textEndDefinition.viewSettings.moveDuration = 0.5f;
	textEndDefinition.viewSettings.holdDuration = 3.5f;
	textEndDefinition.viewSettings.easeType = EaseType::kEaseOutExpo;
	textEndDefinition.showSuccessColor = false;
	textEndDefinition.onCompleted = [this]()
	{
		if (gameFlow_)
		{
			gameFlow_->AdvanceToNextPhase();
		}
	};

	tutorialTextSequence_ = std::make_unique<TutorialTextSequence>(
		gameCamera->GetCamera(),
		gameFlow_,
		lockOnController_,
		inputCommand_,
		std::vector<TutorialTextSequence::StepDefinition>{
			text0Definition,
			text1Definition,
			text2Definition,
			text3Definition,
			text4Definition,
			text5Definition,
			text6Definition,
			text7Definition,
			text8Definition,
			text9Definition,
			text10Definition,
			text11Definition,
			text12Definition,
			text13Definition,
			textEndDefinition });

	energyView_ = gameObjectManager_->AddObject<EnergyView>(
		digitModels,
		mainCamera_.get(),
		rocket_);

	//==============================================
	// これより下はエフェクトのテストで書いています
	//==============================================

	// ブラックホールのテスト
	//auto* sphereModel = modelManager_->GetNameByModel("sphere.obj");
	//auto* ringModel = modelManager_->GetNameByModel("blackHoleRing.gltf");
	//gameObjectManager_->AddObject<BlackHoleEffect>(sphereModel, ringModel);

	//// ポール
	//auto* poleModel = modelManager_->GetNameByModel("pole.gltf");
	//poleModel->SetDefaultIsEnableLight(false);
	//// 円
	auto* circleModel = modelManager_->GetNameByModel("stageCircle.gltf");
	circleModel->SetDefaultIsEnableLight(false);
	// 宇宙を映す平面
	auto* halfDomeModel = modelManager_->GetNameByModel("halfDome.gltf");
	halfDomeModel->SetDefaultIsEnableLight(false);

	// 出現位置のテスト
	auto* ring1Model = modelManager_->GetNameByModel("fieldRingLv1.gltf");
	auto* ring2Model = modelManager_->GetNameByModel("fieldRingLv2.gltf");
	auto* ring3Model = modelManager_->GetNameByModel("fieldRingLv3.gltf");
	gameObjectManager_->AddObject<SpawnFieldEffect>(ring1Model, ring2Model, ring3Model, halfDomeModel, circleModel);

	// エフェクト用モデル
	auto* effectModel = modelManager_->GetNameByModel("plane.obj");
	effectModel->SetDefaultIsEnableLight(false);
	gameObjectManager_->AddObject<ParticleBehavior>("fieldRingOneEffect", 32, textureManager_, effectModel);
	gameObjectManager_->AddObject<ParticleBehavior>("fieldRingTwoEffect", 128, textureManager_, effectModel);
	gameObjectManager_->AddObject<ParticleBehavior>("fieldRingThreeEffect", 128, textureManager_, effectModel);

	// にぎやかし浮遊エフェクト
	gameObjectManager_->AddObject<ParticleBehavior>("fieldFloatingEffect", 256, textureManager_, effectModel);

	uint32_t unitIconGH = textureManager_->GetHandleByName("unitIcon.png");
	gameObjectManager_->AddObject<TimeUI>(unitIconGH);
}

void GameScene::Initialize() {
	mainCamera_->transform_.translate = kCameraPosition;
	mainCameraEndRotation_ = Math::DirectionToEuler(kCameraTarget - kCameraPosition);
	mainCameraDebugParameter_->Apply();
	UpdateCamera();

	score_.Reset();
	scoreView_->SetValue(score_.GetDisplayedValue());

	// 128x72のFade.pngを10倍にして画面全体を覆い、開始時は不透明にする。
	fadeSprite_ = std::make_unique<Sprite>(
		Vector2{ 0.0f, 0.0f },
		kFadeTextureSize,
		Vector2{ 0.0f, 0.0f },
		Vector4{ 1.0f, 1.0f, 1.0f, 1.0f },
		Vector2{ 0.0f, 0.0f },
		kFadeTextureSize,
		kFadeTextureSize);
	fadeSprite_->textureHandle_ = textureManager_->GetHandleByName("Fade.png");
	fadeSprite_->scale_ = kFadeScale;
	fadeSprite_->Update();
	fadeElapsedTime_ = 0.0f;
	if (tutorialLogoView_) tutorialLogoView_->Reset();
	if (tutorialTextSequence_) tutorialTextSequence_->Reset();
	if (startPlayingView_) startPlayingView_->Reset();
}

void GameScene::Update() {
	if (fadeSprite_ && fadeElapsedTime_ < kFadeDuration) {
		fadeElapsedTime_ = std::min(fadeElapsedTime_ + FpsCounter::deltaTime, kFadeDuration);
		const float progress = fadeElapsedTime_ / kFadeDuration;
		fadeSprite_->color_.w = 1.0f - progress;
		fadeSprite_->Update();
	}

	score_.Update(FpsCounter::deltaTime);
	scoreView_->SetValue(score_.GetDisplayedValue());
	scoreView_->Update();
	UpdateTutorialViews(true);
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

	// ライト調整
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("SceneLight");
	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);
	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	ImGui::End();
#endif
}

void GameScene::DebugUpdate()
{
	scoreView_->SetValue(score_.GetDisplayedValue());
	scoreView_->Update();
	UpdateTutorialViews(false);
	UpdateCamera();
	
	// ゲーム更新を停止している間に振動が残らないようにする。
	if (controllerVibration_)
	{
		controllerVibration_->Stop();
	}
}

void GameScene::Draw() {
	DrawTutorialViews();
	if (startPlayingView_) startPlayingView_->Draw(renderQueue_);
	scoreView_->Draw(renderQueue_);
	if (fadeSprite_) {
		renderQueue_->SubmitSprite(fadeSprite_.get());
	}
}

void GameScene::UpdateTutorialViews(bool advanceAnimation)
{
	const IGamePhase* currentPhase = gameFlow_ ? gameFlow_->GetCurrentPhase() : nullptr;
	const bool isTutorial = currentPhase && std::string_view(currentPhase->GetName()) == "Tutorial";
	const float deltaTime = FpsCounter::deltaTime;
	if (tutorialLogoView_) tutorialLogoView_->Update(isTutorial, advanceAnimation, deltaTime);
	if (tutorialTextSequence_) tutorialTextSequence_->Update(advanceAnimation, deltaTime);
}

void GameScene::DrawTutorialViews()
{
	if (tutorialLogoView_) tutorialLogoView_->Draw(renderQueue_);
	if (tutorialTextSequence_) tutorialTextSequence_->Draw(renderQueue_);
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
	mainCameraDebugParameter_->ApplyIfDirty();
	mainCamera_->transform_.rotate = mainCameraEndRotation_;
	if (rocket_)
	{
		// ロケットと同じイージング済み進行率で、X回転を開始角度から着地時の角度へ動かす。
		mainCamera_->transform_.rotate.x = GameEngine::Lerp(
			mainCameraEntranceStartRotateX_,
			mainCameraEndRotation_.x,
			rocket_->GetEntranceProgress());
	}
	mainCamera_->Update();

	// 打ち上げ演出以降はResultMoveCameraがRenderQueueのカメラを管理する。
	if (!gameFlow_ || gameFlow_->UsesGameSceneCamera())
	{
		renderQueue_->SetCamera(mainCamera_.get());
	}
}
