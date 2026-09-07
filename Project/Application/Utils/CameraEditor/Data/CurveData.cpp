#include "CurveData.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace {
	Vector2 CubicBezier(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t) {
		const float u = 1.0f - t;
		return p0 * (u * u * u) + p1 * (3.0f * u * u * t) +
			p2 * (3.0f * u * t * t) + p3 * (t * t * t);
	}
}

float CameraAnimationCurve::Evaluate(float time) const {
	if (keys_.empty()) {
		return 0.0f;
	}
	if (time <= keys_.front().time) {
		return keys_.front().value;
	}
	if (time >= keys_.back().time) {
		return keys_.back().value;
	}

	auto next = std::upper_bound(keys_.begin(), keys_.end(), time,
		[](float value, const CurveKey& key) { return value < key.time; });
	const CurveKey& b = *next;
	const CurveKey& a = *(next - 1);
	if (b.time - a.time <= 0.00001f) {
		return b.value;
	}

	const Vector2 p0 = { a.time, a.value };
	Vector2 p1 = p0 + a.rightHandle;
	const Vector2 p3 = { b.time, b.value };
	Vector2 p2 = p3 + b.leftHandle;
	p1.x = std::clamp(p1.x, p0.x, p3.x);
	p2.x = std::clamp(p2.x, p0.x, p3.x);
	if (p1.x > p2.x) {
		const float middle = (p1.x + p2.x) * 0.5f;
		p1.x = middle;
		p2.x = middle;
	}

	// The Bezier parameter is not time when a handle moves horizontally.
	// Invert BezierX with a binary search, then evaluate BezierY.
	float low = 0.0f;
	float high = 1.0f;
	for (int i = 0; i < 18; ++i) {
		const float t = (low + high) * 0.5f;
		if (CubicBezier(p0, p1, p2, p3, t).x < time) {
			low = t;
		} else {
			high = t;
		}
	}
	return CubicBezier(p0, p1, p2, p3, (low + high) * 0.5f).y;
}

void CameraAnimationCurve::AddKey(const CurveKey& key) {
	CurveKey added = key;
	added.id = nextKeyId_++;
	for (auto& current : keys_) {
		if (std::abs(current.time - added.time) < 0.0001f) {
			added.id = current.id;
			current = added;
			return;
		}
	}
	keys_.push_back(added);
	SortKeys();
}

bool CameraAnimationCurve::RemoveKey(uint32_t id) {
	auto it = std::find_if(keys_.begin(), keys_.end(),
		[id](const CurveKey& key) { return key.id == id; });
	if (it == keys_.end()) {
		return false;
	}
	keys_.erase(it);
	return true;
}

void CameraAnimationCurve::SortKeys() {
	std::sort(keys_.begin(), keys_.end(), [](const CurveKey& a, const CurveKey& b) {
		return a.time < b.time;
	});
}

CurveKey& CameraAnimationCurve::GetKey(uint32_t id) {
	for (auto& key : keys_) {
		if (key.id == id) {
			return key;
		}
	}

	throw std::runtime_error("Key with the specified ID not found.");
}

void CameraAnimationCurve::Save(BinaryManager& binManager) {
	uint32_t keyCount = static_cast<uint32_t>(keys_.size());
	binManager.Register(&keyCount);
	for (uint32_t i = 0; i < keyCount; ++i) {
		binManager.Register(&keys_[i].value);
		binManager.Register(&keys_[i].time);
		binManager.Register(&keys_[i].leftHandle);
		binManager.Register(&keys_[i].rightHandle);
		binManager.Register(&keys_[i].id);
	}
}

void CameraAnimationCurve::Load(BinaryManager& binManager) {
	uint32_t keyCount;
	keyCount = binManager.Reverse<uint32_t>();
	keys_.resize(keyCount);
	for (uint32_t i = 0; i < keyCount; ++i) {
		keys_[i].value = binManager.Reverse<float>();
		keys_[i].time = binManager.Reverse<float>();
		keys_[i].leftHandle = binManager.Reverse<Vector2>();
		keys_[i].rightHandle = binManager.Reverse<Vector2>();
		keys_[i].id = binManager.Reverse<uint32_t>();
		nextKeyId_ = std::max(nextKeyId_, keys_[i].id + 1);
	}
	SortKeys();
}
