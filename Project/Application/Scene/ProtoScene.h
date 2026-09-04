#pragma once

#include "IScene.h"

#include "Camera.h"

#include "Application/Scene/Transition/Fade.h"

namespace Prototype {
	class Field;
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
	void UpdateCamera();
	void DrawOriginGuide();

	bool isFinished_ = false;
	std::unique_ptr<GameEngine::Camera> mainCamera_;
	Prototype::Field* field_ = nullptr;
};
