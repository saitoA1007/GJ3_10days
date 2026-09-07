#pragma once
#include "CurveData.h"

struct CameraCurveData {
	CameraAnimationCurve posXCurve;
	CameraAnimationCurve posYCurve;
	CameraAnimationCurve posZCurve;

	CameraAnimationCurve rotXCurve;
	CameraAnimationCurve rotYCurve;
	CameraAnimationCurve rotZCurve;

	float totalTime = 0.0f;

	void AddKey(Vector3 pos, Vector3 rot, float time);
	void Sort();
	CameraAnimationCurve& GetCurve(size_t index);
	const CameraAnimationCurve& GetCurve(size_t index) const;

	void Save(BinaryManager& binManager);
	void Load(BinaryManager& binManager);

private:

	static inline const std::string key_ = "CurveData";
};
