#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class BlackHoleRingMaterial {
	public:
		struct alignas(16) BlackHoleRingMaterialData {
			Matrix4x4 uvTransform;    // UVの拡大縮小・回転・オフセット

			float innerRadius;       // リング内側の半径（ローカル空間、事象の地平線側）
			float outerRadius;       // リング外側の半径（ローカル空間）
			float time;              // 経過時間
			float scrollSpeed;       // UVスクロール（回転）の速さ

			float noiseScale;        // ノイズのタイリング量（Tilerのタイル数に相当）
			float noiseJitter;       // ノイズの粒のばらつき具合（Rnd Offsetに相当。0〜1）
			float driftSpeed;        // ノイズが時間で揺らぐ速さ
			float dissolveThreshold; // ディゾルブの閾値（0〜1）

			float dissolveEdge;      // ディゾルブ境界のぼかし幅（Gaussian Blurに相当）
			float densityPower;      // 中心に近づくほど密度・発光を強める指数
			float emissionIntensity; // 発光の基準強度
			float pad0;

			Vector3 emissionColor;    // 発光色
			float pad1;
		};

	public:
		BlackHoleRingMaterial();
		~BlackHoleRingMaterial() = default;

		BlackHoleRingMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<BlackHoleRingMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		BlackHoleRingMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<BlackHoleRingMaterialData> materialBuffer_;
	};
}