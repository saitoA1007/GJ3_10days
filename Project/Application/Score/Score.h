#pragma once

class Score {
public:
	void Add(int points);
	void Reset();
	int GetValue() const { return value_; }

private:
	int value_ = 0;
};
