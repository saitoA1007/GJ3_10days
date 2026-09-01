#include "UniverseMaterial.h"
#include "MyMath.h"
using namespace GameEngine;

UniverseMaterial::UniverseMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	// 白色に設定
	materialData_->PlayerPos = {0.0f,0.0f}; // プレイヤーの位置
	materialData_->time = 0.0f; // 時間
	materialData_->radius = 1.0f; // 半径
	materialData_->swirl = 1.0f; // 渦
	materialData_->scale = 2.0f; // サイズ
	materialData_->strength = 1.0f; // サイズ
	materialData_->UniversePos = {0.0f,0.0f}; // サイズ
}