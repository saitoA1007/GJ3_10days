#include "BlackHoleMaterial.h"
#include "MyMath.h"
using namespace GameEngine;

BlackHoleMaterial::BlackHoleMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	materialData_->radius = 2.0f;   // 事象の地平線の半径
	materialData_->strength = 1.0f; // 光を曲げる強さ
	materialData_->swirl = 1.0f;
	materialData_->glowIntensity = 1.0f; // ふちの発光強度
	materialData_->glowColor = {1.0f,1.0f,0.0f}; // ふちの発光色
	materialData_->glowWidth = 0.2f;
}