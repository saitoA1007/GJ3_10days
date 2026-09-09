#include "MonorisManager.h"
#include <RandomGenerator.h>

MonorisManager::MonorisManager(GameEngine::Model* model, uint32_t maxMonorisNum) {
	monoris_.reserve(maxMonorisNum);
	for (uint32_t i = 0; i < maxMonorisNum; ++i) {
		auto& m = monoris_.emplace_back(model);
		m.SetMoveSpeed(RandomGenerator::Get(-1.0f, 1.0f));
	}

	model_ = model;

	debugParameter_.Register("Scale", baseTransform_.scale, 0, "Transform");
	debugParameter_.Register("Rotate", baseTransform_.rotate, 1, "Transform");
	debugParameter_.Register("Translate", baseTransform_.translate, 2, "Transform");
	debugParameter_.Register("Radius", radius_, 3, "Transform");
	debugParameter_.Register("MoveSpeedRatio", moveSpeedRatio_, 4, "Transform");
	debugParameter_.Register("FloatingWidth", floatingWidth_, 5, "Transform");
	debugParameter_.Register("FloatingSpeed", floatingSpeed_, 6, "Transform");
	debugParameter_.Register("Color", color_);

	debugParameter_.Apply();

	Monoris::SetTransform(baseTransform_);
	Monoris::SetRadius(radius_);
	Monoris::SetMoveSpeedRatio(moveSpeedRatio_);
	Monoris::SetFloatingWidth(floatingWidth_);
	Monoris::SetFloatingSpeed(floatingSpeed_);
	Monoris::SetColor(color_);
}

void MonorisManager::Initialize() {
	for(auto& m : monoris_) {
		m.Initialize();
	}
}

void MonorisManager::Update() {
	for (auto& m : monoris_) {
		m.Update();
	}
}

void MonorisManager::Draw() {
	for (auto& m : monoris_) {
		m.Draw();
	}
}

void MonorisManager::DebugUpdate() {
	debugParameter_.ApplyIfDirty();

	Monoris::SetTransform(baseTransform_);
	Monoris::SetRadius(radius_);
	Monoris::SetMoveSpeedRatio(moveSpeedRatio_);
	Monoris::SetFloatingWidth(floatingWidth_);
	Monoris::SetFloatingSpeed(floatingSpeed_);
	Monoris::SetColor(color_);

	//model_->SetDefaultColor(color_, "default");
	//model_->SetDefaultMetallic(metallic_, "default");
	//model_->SetDefaultShininess(roughness_, "default");

	//for (auto& m : monoris_) {
	//	m.DebugUpdate();
	//}
}
