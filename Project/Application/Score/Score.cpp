#include "Score.h"
#include <algorithm>

void Score::Add(int points) {
	if (points == 0) {
		return;
	}

	value_ += points;
	// 連続で加算された場合も、現在の表示値から新しい合計へ進める。
	countUpStartValue_ = displayedValue_;
	countUpElapsed_ = 0.0f;
}

void Score::Update(float deltaTime) {
	const float targetValue = static_cast<float>(value_);
	if (deltaTime <= 0.0f || displayedValue_ == targetValue) {
		return;
	}

	countUpElapsed_ = (std::min)(countUpElapsed_ + deltaTime, kCountUpDuration);
	if (countUpElapsed_ >= kCountUpDuration) {
		displayedValue_ = targetValue;
		return;
	}

	const float progress = countUpElapsed_ / kCountUpDuration;
	displayedValue_ = countUpStartValue_ + (targetValue - countUpStartValue_) * progress;
}

void Score::Reset() {
	value_ = 0;
	displayedValue_ = 0.0f;
	countUpStartValue_ = 0.0f;
	countUpElapsed_ = 0.0f;
}
