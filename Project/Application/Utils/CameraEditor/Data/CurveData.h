#pragma once
#include <Transform.h>
#include <Vector2.h>
#include <Application/Utils/Binary/BinaryManager.h>
#include <vector>

struct CurveKey {
	float value = 0.0f;
	float time = 0.0f;

	Vector2 leftHandle = { -0.2f, 0.0f };
	Vector2 rightHandle = { 0.2f, 0.0f };

	uint32_t id = 0;
};

class CameraAnimationCurve {
public:

	float Evaluate(float time) const;
	void AddKey(const CurveKey& key);
	void SortKeys();

	CurveKey& GetKey(uint32_t id);
	std::vector<CurveKey>& GetKeys() { return keys_; }
	const std::vector<CurveKey>& GetKeys() const { return keys_; }
	bool RemoveKey(uint32_t id);

	void Save(BinaryManager& binManager);
	void Load(BinaryManager& binManager);

private:

	std::vector<CurveKey> keys_;
	uint32_t nextKeyId_ = 0;

};
