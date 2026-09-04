#include "HyperspaceEffect.h"
#include "FPSCounter.h"
using namespace GameEngine;

HyperspaceEffect::HyperspaceEffect(GameEngine::Model* model) : halfDomeModel_(model) {
	// マテリアルを登録
	halfDomeModel_.SetHitGroup(5);
	halfDomeModel_.SetBufferMaterial(0, hyperspaceMaterial_.GetMaterialSrvIndex());

	debugParame_ = std::make_unique<GameEngine::DebugParameter>("HyperspaceEffect");
	debugParame_->RegisterWorld("", halfDomeModel_.worldTransform_);
	debugParame_->Register("HyperspaceJumpDuration", kHyperspaceJumpDuration_);
	debugParame_->Apply();
}

void HyperspaceEffect::Initialize() {

}

void HyperspaceEffect::Update() {
	debugParame_->ApplyIfDirty();

	hyperspaceMaterial_.materialData_->time += FpsCounter::gameDeltaTime;
	if (hyperspacePhase_ != GameEngine::HyperspacePhase::Idle)
	{
		hyperspacePhaseTime_ += FpsCounter::gameDeltaTime / kHyperspaceJumpDuration_;
		hyperspaceMaterial_.materialData_->time += FpsCounter::gameDeltaTime;

		hyperspaceMaterial_.materialData_->phase = static_cast<uint32_t>(hyperspacePhase_);
		hyperspaceMaterial_.materialData_->phaseTime = hyperspacePhaseTime_;
	}

	halfDomeModel_.Update();
}

void HyperspaceEffect::Draw() {
	halfDomeModel_.DrawCustomRaytracing(renderQueue_);
}