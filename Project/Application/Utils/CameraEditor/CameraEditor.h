#pragma once
#include "Data/CameraEditorData.h"
#include <Camera/Camera.h>
#include <Input.h>

class CameraEditor {
public:

	void Update(GameEngine::Input* input, float deltaTime);

	void SetData(const CameraCurveData& data);
	const CameraCurveData& GetData() const { return data_; }

	Transform GetCurrentTransform();

	//カメラの位置座標の逆行列を設定する
	void SetMatrixForGuizmo(Matrix4x4 viewMatrix, Matrix4x4 projectionMatrix) { viewMatrix_ = viewMatrix; projectionMatrix_ = projectionMatrix; }

private:

	void CurveDraw();
	void ApplyCameraAt(float time);
	void CameraGizmo();

	enum class EditorMode {
		kStop,
		kPlay,
	} mode_ = EditorMode::kStop;

	CameraCurveData data_;
	Transform transform_;

	Matrix4x4 viewMatrix_ = Matrix4x4::MakeIdentity();
	Matrix4x4 projectionMatrix_ = Matrix4x4::MakeIdentity();

	float timer_ = 0.0f;
	float viewTimeMin_ = 0.0f;
	float viewTimeMax_ = 5.0f;
	float viewValueMin_ = -5.0f;
	float viewValueMax_ = 5.0f;
	bool curveVisible_[6] = { true, true, true, true, true, true };
	bool fitViewRequested_ = true;
	bool loop_ = true;
	float valuePerGrid_[2] = { 1.0f, 1.0f };
	float valueViewCenter_[2] = { 0.0f, 0.0f };
	int curveGroup_ = 0;
	int selectedCurve_ = 0;
	uint32_t selectedKeyId_ = UINT32_MAX;

	bool isGizmoActive_ = false;

	enum class DragTarget {
		kNone,
		kKey,
		kLeftHandle,
		kRightHandle,
	} dragTarget_ = DragTarget::kNone;

};
