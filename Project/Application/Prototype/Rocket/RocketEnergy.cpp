#include "RocketEnergy.h"

#include <algorithm>
#include <limits>

namespace Prototype {

	RocketEnergy::RocketEnergy(int32_t initialEnergy) {
		Reset(initialEnergy);
	}

	EnergyChange RocketEnergy::Reset(int32_t energy) {
		const int32_t before = current_;
		current_ = std::max(energy, 0);
		return { before, current_ - before, current_, EnergyChangeReason::Reset };
	}

	EnergyChange RocketEnergy::Add(int32_t amount, EnergyChangeReason reason) {
		const int32_t before = current_;
		if (amount <= 0) {
			return { before, 0, before, reason };
		}

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

		const int32_t consumed = std::min(requestedAmount, current_);
		current_ -= consumed;
		return { before, -consumed, current_, reason };
	}
}
