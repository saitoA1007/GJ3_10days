#include "Monoris.h"
#include <FPSCounter.h>
#include <RandomGenerator.h>

Monoris::Monoris(GameEngine::Model* model) {
	if (model == nullptr) {
		throw std::invalid_argument("model cannot be nullptr");
	}
	modelComponent_ = std::make_unique<GameEngine::ModelComponent>(model);
}

void Monoris::Initialize() {
	timer_ = RandomGenerator::Get(-64.f, 8.f);
}

void Monoris::Update() {
	timer_ += GameEngine::FpsCounter::deltaTime;
}

void Monoris::Draw() {
	modelComponent_->Draw(renderQueue_);
}

void Monoris::DebugUpdate() {
	Vector3 localPos = {
		std::cos(timer_ * moveSpeed_ * moveSpeedRatio_) * radius_,
		std::sin(timer_ * floatingSpeed_)* floatingWidth_,
		std::sin(timer_ * moveSpeed_ * moveSpeedRatio_) * radius_
	};
	modelComponent_->worldTransform_.transform_.scale = baseTransform_.scale;
	modelComponent_->worldTransform_.transform_.rotate = baseTransform_.rotate;
	modelComponent_->worldTransform_.transform_.translate = baseTransform_.translate + localPos;
	modelComponent_->Update();
}
