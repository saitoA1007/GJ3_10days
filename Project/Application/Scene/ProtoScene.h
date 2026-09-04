#pragma once

#include "IScene.h"

#include "Camera.h"

#include "Application/Scene/Transition/Fade.h"

namespace Prototype {
	class EnemyManager;
	class EnergySpawner;
	class EnergyView;
	class Field;
	class GameFlowController;
	class LockOnController;
	class Rocket;
	class UnitManager;
}

/// <summary>
/// 機能検証や仮組みに使用するプロトタイプ用シーン
/// </summary>
class ProtoScene final : public GameEngine::IScene {
public:
	ProtoScene();
	~ProtoScene() override = default;

	void Initialize() override;
	void Update() override;
	void DebugUpdate() override;
	void Draw() override;

	bool IsFinished() override { return isFinished_; }
	std::string NextSceneName() override { return "Proto"; }
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:
	void RegisterInputCommands();
	void UpdateCamera();
	void DrawOriginGuide();

	bool isFinished_ = false;
	std::unique_ptr<GameEngine::Camera> mainCamera_;
	Prototype::Field* field_ = nullptr;
	Prototype::Rocket* rocket_ = nullptr;
	Prototype::EnergySpawner* energySpawner_ = nullptr;
	Prototype::UnitManager* unitManager_ = nullptr;
	Prototype::EnemyManager* enemyManager_ = nullptr;
	Prototype::LockOnController* lockOnController_ = nullptr;
	Prototype::GameFlowController* gameFlowController_ = nullptr;
	Prototype::EnergyView* energyView_ = nullptr;
};
