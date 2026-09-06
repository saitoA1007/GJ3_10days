#include "ShuffleNumber.h"
#include <RandomGenerator.h>
#include <ImGuiManager.h>

#include <numbers>

ShuffleNumber::ShuffleNumber(GameEngine::ModelManager* modelManager) {
	const std::string extension = ".obj";

	for (int i = 0; i < 10; ++i) {
		std::string modelFile = std::to_string(i) + extension;
		numberModels_.push_back(modelManager->GetNameByModel(modelFile));
	}
}

ShuffleNumber::~ShuffleNumber() {
}

void ShuffleNumber::Initialize() {
	shuffling_ = false;
	isActive_ = true;

	worldTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
}

void ShuffleNumber::Update() {
	if (shuffling_) {
		int randomIndex = RandomGenerator::Get<int>(0, 9);
		currentIndex_ = randomIndex;
	}
}

void ShuffleNumber::Draw() {
	if (!isActive_) {
		return;
	}

	renderQueue_->SubmitAddModel(numberModels_[currentIndex_], worldTransform_);
}

void ShuffleNumber::DebugUpdate() {
	worldTransform_.UpdateTransformMatrix();
}

void ShuffleNumber::SetTransform(const Transform& transform) {
	worldTransform_.transform_ = transform;
	worldTransform_.transform_.rotate.x += std::numbers::pi_v<float> / 2.0f;
	worldTransform_.transform_.scale.x *= -1.0f;
}

void ShuffleNumber::ShuffleStart() {
	shuffling_ = true;
}

void ShuffleNumber::Stop(int number) {
	shuffling_ = false;
	if (number >= 0 && number <= 9) {
		currentIndex_ = number;
	}
}
