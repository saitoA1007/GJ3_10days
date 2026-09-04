#include "HyperspaceMaterial.h"
using namespace GameEngine;

HyperspaceMaterial::HyperspaceMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	materialData_->time = 0.0f;
	materialData_->phaseTime = 0.0f;
	materialData_->phase = static_cast<uint32_t>(HyperspacePhase::Idle);
	materialData_->pad = 0.0f;
}