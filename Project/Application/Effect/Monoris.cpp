#include "Monoris.h"
#include <FPSCounter.h>
#include <RandomGenerator.h>

Monoris::Monoris(GameEngine::Model* model) {
	if (model == nullptr) {
		throw std::invalid_argument("model cannot be nullptr");
	}
	modelComponent_ = std::make_unique<GameEngine::ModelComponent>(model);
	modelComponent_->materialData_->metallic = 1.5f;
	modelComponent_->materialData_->roughness = -0.04f;

	float scale = RandomGenerator::Get(0.2f, 0.7f);
	modelComponent_->worldTransform_.transform_.scale = { scale,scale,scale };
	modelComponent_->worldTransform_.transform_.rotate.y = RandomGenerator::Get(0.0f, 7.0f);
	rotateSpeed_ = RandomGenerator::Get(-1.0f, 1.0f);
	radiusRatio_ = RandomGenerator::Get(0.9f, 1.1f);
	if (rotateSpeed_ == 0.0f) {
		rotateSpeed_ = 0.1f;
	}
}

void Monoris::Initialize() {
	timer_ = RandomGenerator::Get(-64.f, 8.f);
}

void Monoris::Update() {
	timer_ += GameEngine::FpsCounter::gameDeltaTime;
	modelComponent_->worldTransform_.transform_.rotate.y += rotateSpeed_ * GameEngine::FpsCounter::gameDeltaTime;
	float radius = radius_ * radiusRatio_;
	Vector3 localPos = {
		std::cos(timer_ * moveSpeed_ * moveSpeedRatio_) * radius ,
		std::sin(timer_ * floatingSpeed_) * floatingWidth_,
		std::sin(timer_ * moveSpeed_ * moveSpeedRatio_) * radius
	};
	modelComponent_->worldTransform_.transform_.translate = baseTransform_.translate + localPos;
	modelComponent_->materialData_->color = color_;
	modelComponent_->Update();
}

void Monoris::Draw() {
	modelComponent_->DrawRaytracing(renderQueue_);
}

void Monoris::DebugUpdate() {
	//float radius = radius_ * radiusRatio_;
	//Vector3 localPos = {
	//	std::cos(timer_ * moveSpeed_ * moveSpeedRatio_) * radius ,
	//	std::sin(timer_ * floatingSpeed_)* floatingWidth_,
	//	std::sin(timer_ * moveSpeed_ * moveSpeedRatio_) * radius
	//};
	////modelComponent_->worldTransform_.transform_.scale = baseTransform_.scale;
	////modelComponent_->worldTransform_.transform_.rotate = baseTransform_.rotate;
	//modelComponent_->worldTransform_.transform_.translate = baseTransform_.translate + localPos;
	//modelComponent_->materialData_->color = color_;
	//modelComponent_->Update();
}
