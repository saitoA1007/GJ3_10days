#pragma once
#include "Application/Utils/GameTimer.h"

class Score {
public:
	static constexpr int kMinValue = 0;
	static constexpr int kMaxValue = 99999;

	void Add(int points);
	void Update(float deltaTime);
	void Reset();

	/// @brief 現在のスコア値を取得
	int GetValue() const { return value_; }

	/// @brief 現在の表示値を取得
	int GetDisplayedValue() const { return static_cast<int>(displayedValue_); }

private:
	static constexpr float kCountUpDuration = 0.5f; // 加算表示にかける秒数

	int value_ = 0;
	float displayedValue_ = 0.0f;    // 表示用のスコア値
	float countUpStartValue_ = 0.0f; // 表示値がどこからどこまで進むかを記録
	GameTimer countUpTimer_;
};
