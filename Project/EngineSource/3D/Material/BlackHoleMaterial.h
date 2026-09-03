#pragma once
#include "Vector2.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class BlackHoleMaterial {
	public:
		struct alignas(16) BlackHoleMaterialData {
			float radius;   // 事象の地平線の半径
			float strength; // 光を曲げる強さ
			float swirl;    // 降着円盤のような渦の強さ
			float pad;
		};

	public:
		BlackHoleMaterial();
		~BlackHoleMaterial() = default;

		BlackHoleMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<BlackHoleMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		BlackHoleMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<BlackHoleMaterialData> materialBuffer_;
	};
}