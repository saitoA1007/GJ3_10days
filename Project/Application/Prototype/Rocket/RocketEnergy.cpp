#include "RocketEnergy.h"

#include <algorithm>
#include <limits>

namespace Prototype {

	RocketEnergy::RocketEnergy(int32_t initialEnergy) {
		// 初期化でもResetを通し、0未満を許可しない規則を一か所に集約する。
		Reset(initialEnergy);
	}

	EnergyChange RocketEnergy::Reset(int32_t energy) {
		// 呼び出し側がUI更新に使えるよう、置き換えでも増減差分を返す。
		const int32_t before = current_;
		current_ = std::max(energy, 0);
		return { before, current_ - before, current_, EnergyChangeReason::Reset };
	}

	EnergyChange RocketEnergy::Add(int32_t amount, EnergyChangeReason reason) {
		const int32_t before = current_;
		// 負数による意図しない消費を許可せず、消費はConsumeUpToへ集約する。
		if (amount <= 0) {
			return { before, 0, before, reason };
		}

		// int32_tの上限を超える加算は、残り容量までに制限する。
		const int32_t available = std::numeric_limits<int32_t>::max() - current_;
		const int32_t added = std::min(amount, available);
		current_ += added;
		return { before, added, current_, reason };
	}

	EnergyChange RocketEnergy::ConsumeUpTo(int32_t requestedAmount, EnergyChangeReason reason) {
		const int32_t before = current_;
		if (requestedAmount <= 0) {
			return { before, 0, before, reason };
		}

		// 保有量より大きな要求でも0未満にせず、実際に渡せる量だけを消費する。
		const int32_t consumed = std::min(requestedAmount, current_);
		current_ -= consumed;
		return { before, -consumed, current_, reason };
	}
}
