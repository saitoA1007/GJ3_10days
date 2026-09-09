#include "ShuffleNumber.h"
#include <RandomGenerator.h>
#include <ImGuiManager.h>
#include  <FpsCounter.h>

#include <numbers>

ShuffleNumber::ShuffleNumber(GameEngine::ModelManager* modelManager, int digit) {
	const std::string extension = ".obj";

	for (int i = 0; i < 10; ++i) {
		std::string modelFile = std::to_string(i) + extension;
		numberModels_.push_back(modelManager->GetNameByModel(modelFile));
	}

	timer_ = float(digit) * -0.1f;
}

ShuffleNumber::~ShuffleNumber() {
}

void ShuffleNumber::Initialize() {
	shuffling_ = false;

	worldTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
}

void ShuffleNumber::Update() {
	if (shuffling_) {
		int randomIndex = RandomGenerator::Get<int>(0, 9);
		currentIndex_ = randomIndex;
	}

	timer_ += GameEngine::FpsCounter::deltaTime;
}

void ShuffleNumber::Draw() {
	if (!isActive_) {
		return;
	}

	renderQueue_->SubmitAddModel(numberModels_[currentIndex_], worldTransform_);
}

void ShuffleNumber::DebugUpdate() {
	worldTransform_.transform_ = transform_;
	worldTransform_.transform_.translate.y += std::sin(timer_ * 2.0f) * 0.2f;
	worldTransform_.UpdateTransformMatrix();
}

void ShuffleNumber::SetTransform(const Transform& transform) {
	transform_ = transform;
	transform_.rotate.x += std::numbers::pi_v<float> / 2.0f;
	transform_.scale.x *= -1.0f;
}

void ShuffleNumber::ShuffleStart() {
	shuffling_ = true;
	isActive_ = true;
}

void ShuffleNumber::Stop(int number) {
	shuffling_ = false;
	isActive_ = true;
	if (number >= 0 && number <= 9) {
		currentIndex_ = number;
	} else {
		isActive_ = false;
	}
}
