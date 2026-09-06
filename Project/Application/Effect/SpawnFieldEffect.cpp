#include "SpawnFieldEffect.h"
#include "FPSCounter.h"
using namespace GameEngine;

SpawnFieldEffect::SpawnFieldEffect(GameEngine::Model* ring1Model, GameEngine::Model* ring2Model, GameEngine::Model* ring3Model,
	GameEngine::Model* halfDomeModel, GameEngine::Model* circleModel)
	: universeModel_(halfDomeModel) {

	universeModel_.SetHitGroup(2);
	universeModel_.SetBufferMaterial(0, universeMaterial_.GetMaterialSrvIndex());

	// 円
	circleModels_.reserve(8);
	for (uint32_t i = 0; i < 8; ++i) {
		std::unique_ptr<ModelComponent> circle = std::make_unique<ModelComponent>(circleModel);

		circle->worldTransform_.transform_.scale = { 35.0f,0.1f,35.0f };
		circle->worldTransform_.transform_.translate = { 0.0f,i * -5.0f,0.0f };

		circle->worldTransform_.UpdateTransformMatrix();
		circleModels_.push_back(std::move(circle));
	}

	// メモリを確保
	materials_.resize(3);
	glowColors_.resize(3);

	ringModels_.reserve(3);
	for (uint32_t i = 0; i < 3; ++i) {
		std::unique_ptr<ModelComponent> model;
		std::unique_ptr<ModelComponent> underModel;

		if (i == 0) {
			model = std::make_unique<ModelComponent>(ring1Model);
			underModel = std::make_unique<ModelComponent>(ring1Model);
		} else if (i == 1) {
			model = std::make_unique<ModelComponent>(ring2Model);
			underModel = std::make_unique<ModelComponent>(ring2Model);
		} else {
			model = std::make_unique<ModelComponent>(ring3Model);
			underModel = std::make_unique<ModelComponent>(ring3Model);
		}
		model->worldTransform_.transform_.translate = {};

		underModel->worldTransform_.transform_.translate.y = -0.1f;
		underModel->materialData_->color = { 0.0f,0.0f,0.0f,1.0f };
		underModel->materialData_->enableLighting = false;
		
		model->SetHitGroup(4);
		model->SetBufferMaterial(0, materials_[i].GetMaterialSrvIndex());
		ringModels_.push_back(std::move(model));
		underRingModels_.push_back(std::move(underModel));
	}

	// 登録
	Register();

	for (uint32_t i = 0; i < 3; ++i) {
		materials_[i].materialData_->emissionColor = Vector3(glowColors_[i].x, glowColors_[i].y, glowColors_[i].z);
	}
}

void SpawnFieldEffect::Initialize() {

}

void SpawnFieldEffect::Update() {
	if (debugParame_->ApplyIfDirty()) {
		for (uint32_t i = 0; i < 3; ++i) {
			materials_[i].materialData_->emissionColor = Vector3(glowColors_[i].x, glowColors_[i].y, glowColors_[i].z);
		}
	}

	// 時間を更新
	universeMaterial_.materialData_->time += FpsCounter::gameDeltaTime;
	for (uint32_t i = 0; i < 3; ++i) {
		materials_[i].materialData_->time += FpsCounter::gameDeltaTime;
	}

	for (uint32_t i = 0; i < 3; ++i) {
		ringModels_[i]->Update();
		underRingModels_[i]->Update();
	}

	// 色を切り替え
	colorTime_ += FpsCounter::gameDeltaTime;
	for (uint32_t i = 0; i < circleModels_.size(); ++i) {
		float h = colorTime_ / colorCycle_ + colorOffset_ * i;
		Vector3 rgb = Math::HSVtoRGB(h, 1.0f, 1.0f);
		circleModels_[i]->materialData_->color = { rgb.x, rgb.y, rgb.z, 1.0f };
	}

	universeModel_.Update();
}

void SpawnFieldEffect::Draw() {

	universeModel_.DrawCustomRaytracing(renderQueue_);

	for (uint32_t i = 0; i < 3; ++i) {
		ringModels_[i]->DrawCustomRaytracing(renderQueue_);
		underRingModels_[i]->DrawRaytracing(renderQueue_);
	}

	// 円
	//for (auto& circle : circleModels_) {
	//	circle->DrawRaytracing(renderQueue_);
	//}
}

void SpawnFieldEffect::Register() {
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("SpawnFieldEffect");
	// エネルギーのスポーン演出
	for (uint32_t i = 0; i < 3; ++i) {
		std::string subGroup = "Ring" + std::to_string(i);
		debugParame_->Register("InnerRadius", materials_[i].materialData_->innerRadius, 0, subGroup);
		debugParame_->Register("OuterRadius", materials_[i].materialData_->outerRadius, 1, subGroup);
		debugParame_->Register("ScrollSpeed", materials_[i].materialData_->scrollSpeed, 2, subGroup);
		debugParame_->Register("noiseScale", materials_[i].materialData_->noiseScale, 3, subGroup);
		debugParame_->Register("noiseJitter", materials_[i].materialData_->noiseJitter, 4, subGroup);
		debugParame_->Register("driftSpeed", materials_[i].materialData_->driftSpeed, 5, subGroup);
		debugParame_->Register("dissolveThreshold", materials_[i].materialData_->dissolveThreshold, 6, subGroup);
		debugParame_->Register("dissolveEdge", materials_[i].materialData_->dissolveEdge, 7, subGroup);
		debugParame_->Register("densityPower", materials_[i].materialData_->densityPower, 8, subGroup);
		debugParame_->Register("emissionIntensity", materials_[i].materialData_->emissionIntensity, 9, subGroup);
		debugParame_->Register("glowColor", glowColors_[i], 10, subGroup);
	}
	// 宇宙
	debugParame_->Register("radius", universeMaterial_.materialData_->radius, 1, "UniverseMaterial");
	debugParame_->Register("swirl", universeMaterial_.materialData_->swirl, 1, "UniverseMaterial");
	debugParame_->Register("scale", universeMaterial_.materialData_->scale, 1, "UniverseMaterial");
	debugParame_->Register("strength", universeMaterial_.materialData_->strength, 1, "UniverseMaterial");
	debugParame_->Register("UniversePos", universeMaterial_.materialData_->UniversePos, 1, "UniverseMaterial");
	debugParame_->Register("Pos", universeModel_.worldTransform_.transform_.translate, 1, "UniverseTransform");
	debugParame_->Register("scale", universeModel_.worldTransform_.transform_.scale, 1, "UniverseTransform");
	// 色の遷移
	debugParame_->Register("Cycle", colorCycle_, 1, "Color");
	debugParame_->Register("Offset", colorOffset_, 1, "Color");
	debugParame_->Apply();
}
