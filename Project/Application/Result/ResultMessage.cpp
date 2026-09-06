#include "ResultMessage.h"
#include <FPSCounter.h>

ResultMessage::ResultMessage(GameEngine::ModelManager* modelManager, GameEngine::AnimationManager* animationManager) {
	int typeCount = static_cast<int>(Type::Count);
	const std::vector<std::string> modelNames = {
		"mousukosi",
		"tyakuriku",
		"tobisugi"
	};
	const std::vector<int> modelCounts = { 5, 5, 4 }; // 各タイプのモデル数を指定
	const int biggestModelCount = *std::max_element(modelCounts.begin(), modelCounts.end());

	resultDataList_.resize(typeCount);
	
	for (int i = 0; i < typeCount; ++i) {
		auto& resultData = resultDataList_[i];
		resultData.models.resize(modelCounts[i]);

		for (int j = 0; j < modelCounts[i]; ++j) {
			std::string modelName = modelNames[i] + std::to_string(j) + ".gltf";
			resultData.models[j] = modelManager->GetNameByModel(modelName);
		}
		resultData.charAnimation = animationManager->GetNameByAnimation(modelNames[i]);
	}

	worldTransforms_.resize(biggestModelCount);
	for (auto& worldTransform : worldTransforms_) {
		worldTransform = std::make_unique<GameEngine::WorldTransform>();
		worldTransform->Initialize({});
	}

	debugParameter_.Register("Scale", transform_.scale, 0, "Transform");
	debugParameter_.Register("Rotate", transform_.rotate, 1, "Transform");
	debugParameter_.Register("Translate", transform_.translate, 2, "Transform");
	debugParameter_.Register("Mergin", translateMergin_, 3, "Transform");

	debugParameter_.Register("Timer", timer_, 0, "Animation");
	debugParameter_.Register("Diray", diray_, 1, "Animation");
}

void ResultMessage::Initialize() {
	timer_ = 0.0f;
	isBoot_ = false;

	debugParameter_.Apply();

	//debug用
	Boot(Type::Tobisugi);
}

void ResultMessage::Update() {
	if (!isBoot_) {
		return;
	}

	auto& data = resultDataList_[static_cast<int>(currentType_)];
	int charCount = static_cast<int>(data.models.size());

	timer_ += GameEngine::FpsCounter::deltaTime;

	for (int i = 0; i < charCount; ++i) {
		animators_[i].Update(std::clamp(timer_ - diray_ * float(i), 0.0f, data.charAnimation.duration));
	}
}

void ResultMessage::Draw() {
	if (!isBoot_) {
		return;
	}

	for (int i = 0; i < resultDataList_[static_cast<int>(currentType_)].models.size(); ++i) {
		renderQueue_->SubmitAddModel(resultDataList_[static_cast<int>(currentType_)].models[i], *worldTransforms_[i].get());
	}
}

void ResultMessage::DebugUpdate() {
	debugParameter_.ApplyIfDirty();

	auto& data = resultDataList_[static_cast<int>(currentType_)];
	int charCount = static_cast<int>(data.models.size());
	float minCharPos = float(charCount - 1) * 0.5f * -translateMergin_;

	float floatingY = sinf(timer_ * floatingSpeed_) * 0.1f;

	for (int i = 0; i < charCount; ++i) {
		Transform& transform = worldTransforms_[i]->transform_;
		transform.scale = transform_.scale;
		transform.scale.x *= -1.f; // 左右反転(原因不明)
		transform.rotate = transform_.rotate;
		transform.translate = transform_.translate + Vector3{ minCharPos + i * translateMergin_, floatingY, 0.0f };
		worldTransforms_[i]->UpdateTransformMatrix();
	}
}

void ResultMessage::Boot(Type type) {
	isBoot_ = true;
	currentType_ = type;

	auto& data = resultDataList_[static_cast<int>(currentType_)];
	animators_.resize(data.models.size());
	for (int i = 0; i < (int)data.models.size(); ++i) {
		animators_[i].Initialize(data.models[i], &data.charAnimation);
	}
}
