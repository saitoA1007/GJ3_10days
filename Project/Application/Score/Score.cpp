#include "Score.h"

void Score::Add(int points) {
	value_ += points;
}

void Score::Reset() {
	value_ = 0;
}