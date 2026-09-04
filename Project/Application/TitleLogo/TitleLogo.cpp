#include "TitleLogo.h"

#include <cassert>
#include <numbers>
#include <string>

#include "Model.h"
#include "ModelComponent.h"
#include "ModelManager.h"
#include "RenderQueue.h"

using namespace GameEngine;

TitleLogo::TitleLogo(ModelManager* modelManager) {
	assert(modelManager);

	constexpr float kScale = 0.75f;
	constexpr float kPartSpacing = 1.9f;
	constexpr float kFirstPartX = -2.85f;
	constexpr float kLogoY = 1.7f;
	constexpr float kLogoZ = -5.0f;
	const Vector3 rotation = {
		std::numbers::pi_v<float> * 0.5f,
		std::numbers::pi_v<float>,
		0.0f
	};

	// 4文字をまとめる下地を、文字より少し奥に配置する。
	Model* bottomModel = modelManager->GetNameByModel("bottom.obj");
	assert(bottomModel && "Title logo model bottom.obj must be loaded.");
	if (bottomModel) {
		// bottom.objが使用する全マテリアルのライティングを無効化する。
		bottomModel->SetDefaultIsEnableLight(false, "TitleLogo");
		bottomModel->SetDefaultIsEnableLight(false, "Material.001");
		bottom_ = std::make_unique<ModelComponent>(bottomModel);
		bottom_->worldTransform_.Initialize({
			{1.0f, 1.0f, 1.0f},
			rotation,
			{0.0f, kLogoY, kLogoZ + 0.1f}
		});
	}

	for (std::size_t i = 0; i < parts_.size(); ++i) {
		const std::string modelName = "t" + std::to_string(i) + ".obj";
		Model* model = modelManager->GetNameByModel(modelName);
		assert(model && "Title logo models t0.obj through t3.obj must be loaded.");
		if (!model) {
			continue;
		}

		model->SetDefaultIsEnableLight(false);
		parts_[i] = std::make_unique<ModelComponent>(model);
		parts_[i]->worldTransform_.Initialize({
			{kScale, kScale, kScale},
			rotation,
			{kFirstPartX + kPartSpacing * static_cast<float>(i), kLogoY, kLogoZ}
		});
	}

	// ImGuiから各モデルのSRTを個別に操作できるよう登録する。
	auto registerSrt = [this](const std::string& groupName, ModelComponent& component) {
		Transform& transform = component.worldTransform_.transform_;
		debugParameter_.Register("Scale", transform.scale, 0, groupName);
		debugParameter_.Register("Rotate", transform.rotate, 1, groupName);
		debugParameter_.Register("Translate", transform.translate, 2, groupName);
		};

	if (bottom_) {
		registerSrt("Bottom", *bottom_);
	}
	for (std::size_t i = 0; i < parts_.size(); ++i) {
		if (parts_[i]) {
			registerSrt("T" + std::to_string(i), *parts_[i]);
		}
	}
	debugParameter_.Apply();
}

TitleLogo::~TitleLogo() = default;

void TitleLogo::Update() {
	debugParameter_.ApplyIfDirty();

	if (bottom_) {
		bottom_->Update();
	}

	for (const auto& part : parts_) {
		if (part) {
			part->Update();
		}
	}
}

void TitleLogo::Draw(RenderQueue* renderQueue) {
	if (!renderQueue) {
		return;
	}

	if (bottom_) {
		bottom_->Draw(renderQueue);
	}

	for (const auto& part : parts_) {
		if (part) {
			part->Draw(renderQueue);
		}
	}
}
