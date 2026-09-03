#pragma once

class Score {
public:
	void Add(int points);
	void Update(float deltaTime);
	void Reset();
	// 確定スコアと、カウントアップ中の表示値を分けて取得する。
	int GetValue() const { return value_; }
	int GetDisplayedValue() const { return static_cast<int>(displayedValue_); }

private:
	static constexpr float kCountUpDuration = 0.5f; // 加算表示にかける秒数

	int value_ = 0;
	float displayedValue_ = 0.0f;
	float countUpStartValue_ = 0.0f;
	float countUpElapsed_ = 0.0f;
};
