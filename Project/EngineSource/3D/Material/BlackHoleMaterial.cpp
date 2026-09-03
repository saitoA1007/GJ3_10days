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
}