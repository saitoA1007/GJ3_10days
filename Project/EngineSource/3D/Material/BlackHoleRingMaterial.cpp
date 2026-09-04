#include "BlackHoleRingMaterial.h"
#include "MyMath.h"
using namespace GameEngine;

BlackHoleRingMaterial::BlackHoleRingMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	materialData_->uvTransform = Matrix4x4::MakeIdentity();    // UVの拡大縮小・回転・オフセット

	materialData_->innerRadius = 1.0f;       // リング内側の半径（ローカル空間、事象の地平線側）
	materialData_->outerRadius = 2.0f;       // リング外側の半径（ローカル空間）
	materialData_->time = 0.0f;              // 経過時間
	materialData_->scrollSpeed = 1.0f;       // UVスクロール（回転）の速さ

	materialData_->noiseScale = 1.0f;        // ノイズのタイリング量（Tilerのタイル数に相当）
	materialData_->noiseJitter = 1.0f;       // ノイズの粒のばらつき具合（Rnd Offsetに相当。0〜1）
	materialData_->driftSpeed = 1.0f;        // ノイズが時間で揺らぐ速さ
	materialData_->dissolveThreshold = 0.5f; // ディゾルブの閾値（0〜1）

	materialData_->dissolveEdge = 1.0f;      // ディゾルブ境界のぼかし幅（Gaussian Blurに相当）
	materialData_->densityPower = 1.0f;      // 中心に近づくほど密度・発光を強める指数
	materialData_->emissionIntensity = 1.0f; // 発光の基準強度

	materialData_->emissionColor = {1.0f,1.0f,0.0f};    // 発光色
}