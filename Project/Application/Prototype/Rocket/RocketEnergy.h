#pragma once

#include <cstdint>

namespace Prototype {

	/// <summary>
	/// エネルギーが変化した理由
	/// </summary>
	enum class EnergyChangeReason : uint8_t {
		Delivery,       // 配達による増加
		UnitAllocation, // ユニット割り当てによる消費
		EnemyHit,       // 敵の攻撃による消費
		Debug,          // デバッグによる増減
		Reset,          // リセットによる増減
	};

	/// <summary>
	/// 一度のエネルギー増減結果
	/// </summary>
	struct EnergyChange {
		int32_t before = 0;
		int32_t amount = 0;
		int32_t after = 0;
		EnergyChangeReason reason = EnergyChangeReason::Delivery;

		bool Changed() const { return amount != 0; }
	};

	/// <summary>
	/// ロケットが保有するエネルギーの値だけを管理する
	/// </summary>
	class RocketEnergy final {
	public:
		explicit RocketEnergy(int32_t initialEnergy = 0);

		EnergyChange Reset(int32_t energy = 0);

		EnergyChange Add(int32_t amount, EnergyChangeReason reason);
		EnergyChange ConsumeUpTo(int32_t requestedAmount, EnergyChangeReason reason);

		int32_t GetCurrent() const { return current_; }
		bool IsEmpty() const { return current_ == 0; }

	private:
		int32_t current_ = 0;
	};
}
