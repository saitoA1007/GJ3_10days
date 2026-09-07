#include "CameraEditorData.h"

void CameraCurveData::AddKey(Vector3 pos, Vector3 rot, float time) {
	CurveKey key;
	key.time = time;
	key.leftHandle = { -0.2f, 0.0f };
	key.rightHandle = { 0.2f, 0.0f };

	auto addKey = [&](CameraAnimationCurve& curve, float value) {
		key.value = value;
		curve.AddKey(key);
		};

	addKey(posXCurve, pos.x);
	addKey(posYCurve, pos.y);
	addKey(posZCurve, pos.z);
	addKey(rotXCurve, rot.x);
	addKey(rotYCurve, rot.y);
	addKey(rotZCurve, rot.z);

	if (totalTime < time) {
		totalTime = time;
	}
}

CameraAnimationCurve& CameraCurveData::GetCurve(size_t index) {
	CameraAnimationCurve* curves[] = {
		&posXCurve, &posYCurve, &posZCurve,
		&rotXCurve, &rotYCurve, &rotZCurve
	};
	return *curves[index];
}

const CameraAnimationCurve& CameraCurveData::GetCurve(size_t index) const {
	const CameraAnimationCurve* curves[] = {
		&posXCurve, &posYCurve, &posZCurve,
		&rotXCurve, &rotYCurve, &rotZCurve
	};
	return *curves[index];
}

void CameraCurveData::Sort() {
	posXCurve.SortKeys();
	posYCurve.SortKeys();
	posZCurve.SortKeys();
	rotXCurve.SortKeys();
	rotYCurve.SortKeys();
	rotZCurve.SortKeys();

	float lastKeyTime = 0.0f;
	for (size_t i = 0; i < 6; ++i) {
		const auto& keys = GetCurve(i).GetKeys();
		if (!keys.empty()) {
			lastKeyTime = std::max(lastKeyTime, keys.back().time);
		}
	}
	totalTime = std::max(totalTime, lastKeyTime);
}

void CameraCurveData::Save(BinaryManager& binManager) {
	binManager.Register(&key_);
	binManager.Register(&totalTime);
	posXCurve.Save(binManager);
	posYCurve.Save(binManager);
	posZCurve.Save(binManager);
	rotXCurve.Save(binManager);
	rotYCurve.Save(binManager);
	rotZCurve.Save(binManager);
}

void CameraCurveData::Load(BinaryManager& binManager) {
	std::string key = binManager.Reverse<std::string>();
	if (key != key_) {
		binManager.Back();
		return;
	}

	totalTime = binManager.Reverse<float>();
	posXCurve.Load(binManager);
	posYCurve.Load(binManager);
	posZCurve.Load(binManager);
	rotXCurve.Load(binManager);
	rotYCurve.Load(binManager);
	rotZCurve.Load(binManager);
}
