#pragma once
#include "Matrix4x4.h"
#include "Vector4.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class CrystalMaterial {
	public:
		struct alignas(16) CrystalMaterialData {
			Matrix4x4 uvTransform;      // UVの拡大縮小・回転・オフセット

			Vector4 baseColor;          // クリスタルの基本色
			Vector4 rimColor;           // 輪郭の発光色
			Vector4 dissolveEdgeColor;  // ディゾルブ境界の発光色

			float time;                 // 経過時間
			float dissolveThreshold;    // 0=無傷、1=完全に消える
			float dissolveEdgeWidth;    // ディゾルブ境界のぼかし幅
			float dissolveNoiseScale;   // ディゾルブノイズのタイリング量

			float rimPower;             // 輪郭発光の絞り具合。大きいほど縁だけ光る
			float rimIntensity;         // 輪郭発光の強さ
			float universeIntensity;    // 映り込む宇宙の強さ
			float universeScale;        // 星のスケール

			float ior;                  // 屈折率
			float fresnelStrength;      // 表面反射の効き具合
			uint32_t textureHandle;     // アルベドテクスチャ
			uint32_t dissolveTextureHandle; // ディゾルブ用テクスチャ。0ならプロシージャルノイズ
		};

	public:
		CrystalMaterial();
		~CrystalMaterial() = default;

		CrystalMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<CrystalMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		CrystalMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<CrystalMaterialData> materialBuffer_;
	};
}
