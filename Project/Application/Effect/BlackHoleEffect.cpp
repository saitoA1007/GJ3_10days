#include "BlackHoleEffect.h"
#include "FPSCounter.h"
using namespace GameEngine;

BlackHoleEffect::BlackHoleEffect(GameEngine::Model* sphereModel, GameEngine::Model* ringModel)
	: sphereModel_(sphereModel), ringModel_(ringModel){

	baseWorld_.transform_.translate = { 0.0f,0.0f,0.0f };
	baseWorld_.transform_.scale = { 1.0f,1.0f,1.0f };

	sphereModel_.worldTransform_.SetParent(&baseWorld_);
	ringModel_.worldTransform_.SetParent(&baseWorld_);
	
	sphereModel_.SetHitGroup(3);
	sphereModel_.SetBufferMaterial(0, blackHoleMaterial_.GetMaterialSrvIndex());

	ringModel_.SetHitGroup(4);
	ringModel_.SetBufferMaterial(0, blackHoleRingMaterial_.GetMaterialSrvIndex());

	// 登録
	Register();

	blackHoleMaterial_.materialData_->glowColor = Vector3(glowColor_.x, glowColor_.y, glowColor_.z);
	blackHoleRingMaterial_.materialData_->emissionColor = Vector3(ringGlowColor_.x, ringGlowColor_.y, ringGlowColor_.z);
}

void BlackHoleEffect::Initialize() {

}

void BlackHoleEffect::Update() {
	if (debugParame_->ApplyIfDirty()) {
		blackHoleMaterial_.materialData_->glowColor = Vector3(glowColor_.x, glowColor_.y, glowColor_.z);
		blackHoleRingMaterial_.materialData_->emissionColor = Vector3(ringGlowColor_.x, ringGlowColor_.y, ringGlowColor_.z);
	}

	// 時間を更新
	blackHoleRingMaterial_.materialData_->time += FpsCounter::gameDeltaTime;

	baseWorld_.UpdateTransformMatrix();
	sphereModel_.Update();
	ringModel_.Update();
}

void BlackHoleEffect::Draw() {
	sphereModel_.DrawCustomRaytracing(renderQueue_);
	ringModel_.DrawCustomRaytracing(renderQueue_);
}

void BlackHoleEffect::Register() {
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("BlackHoleEffect");
	std::string subGroup = "Hole";
	debugParame_->Register("translate", sphereModel_.worldTransform_.transform_.translate, 0, subGroup);
	debugParame_->Register("scale", sphereModel_.worldTransform_.transform_.scale, 0, subGroup);
	debugParame_->Register("radius", blackHoleMaterial_.materialData_->radius, 0, subGroup);
	debugParame_->Register("strength", blackHoleMaterial_.materialData_->strength, 1, subGroup);
	debugParame_->Register("swirl", blackHoleMaterial_.materialData_->swirl, 2, subGroup);
	debugParame_->Register("glowIntensity", blackHoleMaterial_.materialData_->glowIntensity, 3, subGroup);
	debugParame_->Register("glowColor", glowColor_, 4, subGroup);
	debugParame_->Register("glowWidth", blackHoleMaterial_.materialData_->glowWidth, 5, subGroup);
	subGroup = "Ring";
	debugParame_->Register("translate", ringModel_.worldTransform_.transform_.translate, 0, subGroup);
	debugParame_->Register("scale", ringModel_.worldTransform_.transform_.scale, 0, subGroup);
	debugParame_->Register("InnerRadius", blackHoleRingMaterial_.materialData_->innerRadius, 0, subGroup);
	debugParame_->Register("OuterRadius", blackHoleRingMaterial_.materialData_->outerRadius, 1, subGroup);
	debugParame_->Register("ScrollSpeed", blackHoleRingMaterial_.materialData_->scrollSpeed, 2, subGroup);
	debugParame_->Register("noiseScale", blackHoleRingMaterial_.materialData_->noiseScale, 3, subGroup);
	debugParame_->Register("noiseJitter", blackHoleRingMaterial_.materialData_->noiseJitter, 4, subGroup);
	debugParame_->Register("driftSpeed", blackHoleRingMaterial_.materialData_->driftSpeed, 5, subGroup);
	debugParame_->Register("dissolveThreshold", blackHoleRingMaterial_.materialData_->dissolveThreshold, 6, subGroup);
	debugParame_->Register("dissolveEdge", blackHoleRingMaterial_.materialData_->dissolveEdge, 7, subGroup);
	debugParame_->Register("densityPower", blackHoleRingMaterial_.materialData_->densityPower, 8, subGroup);
	debugParame_->Register("emissionIntensity", blackHoleRingMaterial_.materialData_->emissionIntensity, 9, subGroup);
	debugParame_->Register("glowColor", ringGlowColor_, 10, subGroup);
	debugParame_->Apply();
}
