#pragma once
#include "Monoris.h"
#include <IGameObject.h>
#include <DebugParameter.h>
#include <ModelComponent.h>

class MonorisManager : public GameEngine::IGameObject {
public:

	MonorisManager(GameEngine::Model* model, uint32_t maxMonorisNum);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

private:

	GameEngine::Model* model_ = nullptr;

	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float metallic_ = 0.0f;
	float roughness_ = 0.5f;

	GameEngine::DebugParameter debugParameter_{ "MonorisManager" };

	std::vector<Monoris> monoris_;

	//設定項目↓↓
	Transform baseTransform_;
	float radius_ = 0.0f;
	float moveSpeedRatio_ = 1.0f;
	float floatingWidth_ = 2.0f;
	float floatingSpeed_ = 2.0f;

};
