#pragma once
#include "Vector2.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class UniverseMaterial {
	public:
		struct alignas(16) UniverseMaterialData {
			Vector2 PlayerPos; // プレイヤーの位置
			float time; // 時間
			float radius; // 半径

			float swirl; // 渦
			float scale; // サイズ
			float strength;
			float pad;

			Vector2 UniversePos;
			Vector2 pad1;
		};

	public:
		UniverseMaterial();
		~UniverseMaterial() = default;

		UniverseMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<UniverseMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		UniverseMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<UniverseMaterialData> materialBuffer_;
	};
}