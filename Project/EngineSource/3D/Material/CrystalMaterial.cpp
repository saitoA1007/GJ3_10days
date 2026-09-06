#include "CrystalMaterial.h"
#include "MyMath.h"
using namespace GameEngine;

CrystalMaterial::CrystalMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	materialData_->uvTransform = Matrix4x4::MakeIdentity();      // UVの拡大縮小・回転・オフセット

	materialData_->baseColor = { 0.05f,0.05f,0.12f,1.0f };       // クリスタルの基本色
	materialData_->rimColor = { 0.45f,0.75f,1.0f,1.0f };         // 輪郭の発光色
	materialData_->dissolveEdgeColor = { 1.0f,0.55f,0.15f,1.0f };// ディゾルブ境界の発光色

	materialData_->time = 0.0f;                 // 経過時間
	materialData_->dissolveThreshold = 0.0f;    // 0=無傷、1=完全に消える
	materialData_->dissolveEdgeWidth = 0.05f;   // ディゾルブ境界のぼかし幅
	materialData_->dissolveNoiseScale = 3.0f;   // ディゾルブノイズのタイリング量

	materialData_->rimPower = 3.0f;             // 輪郭発光の絞り具合
	materialData_->rimIntensity = 1.5f;         // 輪郭発光の強さ
	materialData_->universeIntensity = 1.2f;    // 映り込む宇宙の強さ
	materialData_->universeScale = 400.0f;      // 星のスケール

	materialData_->ior = 1.45f;                 // 屈折率
	materialData_->fresnelStrength = 1.0f;      // 表面反射の効き具合
	materialData_->textureHandle = 0;           // アルベドテクスチャ
	materialData_->dissolveTextureHandle = 0;   // ディゾルブ用テクスチャ
}
