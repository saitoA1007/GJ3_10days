#pragma once
#include "StructuredBuffer.h"

namespace GameEngine {

	// ハイパースペース演出のフェーズ
	enum class HyperspacePhase : uint32_t {
		Idle = 0, // 非アクティブ
		Jump = 1, // 突入
	};

	class HyperspaceMaterial {
	public:
		struct alignas(16) HyperspaceMaterialData {
			float time; // 星空のアニメーション
			float phaseTime; // 現在のフェーズが始まってからの経過時間
			uint32_t phase; // HyperspacePhase
			float pad;
		};

	public:
		HyperspaceMaterial();
		~HyperspaceMaterial() = default;

		HyperspaceMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<HyperspaceMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		HyperspaceMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<HyperspaceMaterialData> materialBuffer_;
	};
}