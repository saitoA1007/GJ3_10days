#include "Score.h"
#include <algorithm>

void Score::Add(int points) {
	// 加算前に差分を制限
	points = std::clamp(points, kMinValue - value_, kMaxValue - value_);
	if (points == 0) {
		return;
	}

	value_ += points;

	// 連続で加算された場合も、現在の表示値から新しい合計へ進める
	countUpStartValue_ = displayedValue_;
	countUpTimer_.Start(kCountUpDuration);
}

void Score::Update(float deltaTime) {
	if (deltaTime <= 0.0f || !countUpTimer_.IsActive()) {
		return;
	}

	countUpTimer_.Update(deltaTime);
	const float targetValue = static_cast<float>(value_);
	if (countUpTimer_.IsFinished()) {
		displayedValue_ = targetValue;
		return;
	}

	displayedValue_ = countUpStartValue_ + (targetValue - countUpStartValue_) * countUpTimer_.GetProgress();
}

void Score::Reset() {
	value_ = 0;
	displayedValue_ = 0.0f;
	countUpStartValue_ = 0.0f;
	countUpTimer_.Reset();
}
