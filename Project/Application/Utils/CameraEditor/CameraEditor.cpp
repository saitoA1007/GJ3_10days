#include "CameraEditor.h"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <numbers>
#include <ImGuiManager.h>
#include <MyMath.h>

namespace {
	constexpr const char* kCurveNames[6] = {
		"Position X", "Position Y", "Position Z",
		"Rotation X", "Rotation Y", "Rotation Z"
	};

#ifdef USE_IMGUI
	constexpr ImU32 kCurveColors[6] = {
		IM_COL32(235, 80, 80, 255), IM_COL32(90, 210, 100, 255), IM_COL32(80, 135, 245, 255),
		IM_COL32(245, 145, 90, 255), IM_COL32(120, 235, 185, 255), IM_COL32(180, 115, 245, 255)
	};

	float DistanceSquared(ImVec2 a, ImVec2 b) {
		const float x = a.x - b.x;
		const float y = a.y - b.y;
		return x * x + y * y;
	}
#endif
}

void CameraEditor::SetData(const CameraCurveData& data) {
	data_ = data;
	data_.Sort();
	timer_ = 0.0f;
	selectedKeyId_ = UINT32_MAX;
	fitViewRequested_ = true;
}

void CameraEditor::Update(GameEngine::Input* input, float deltaTime) {
	bool add = false;
	bool applyCamera = false;

#ifdef USE_IMGUI
	ImGui::Begin("Camera Editor");
	if (ImGui::Button("Stop")) {
		mode_ = EditorMode::kStop;
		timer_ = 0.0f;
		applyCamera = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(mode_ == EditorMode::kPlay ? "Pause" : "Play")) {
		mode_ = mode_ == EditorMode::kPlay ? EditorMode::kStop : EditorMode::kPlay;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Key")) {
		add = true;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Loop", &loop_);

	float duration = std::max(data_.totalTime, 0.01f);
	ImGui::SetNextItemWidth(120.0f);
	{
		float tmp = duration;
		ImGui::DragFloat("Duration", &duration, 0.01f, 0.01f, 3600.0f, "%.2f s");
		if (duration != tmp) {
			data_.totalTime = duration;
			timer_ = std::min(timer_, data_.totalTime);
			applyCamera = true;
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderFloat("##CameraTime", &timer_, 0.0f, duration, "%.3f s")) {
		applyCamera = true;
	}
	CurveDraw();
	ImGui::End();
#endif

	if (input) {
		if (input->TriggerKey(DIK_I)) {
			add = true;
		}
		if (input->TriggerKey(DIK_SPACE)) {
			mode_ = mode_ == EditorMode::kStop ? EditorMode::kPlay : EditorMode::kStop;
		}
		if (input->TriggerKey(DIK_F1)) {
			isGizmoActive_ = !isGizmoActive_;
		}
	}

	if (add) {
		data_.AddKey(transform_.translate, transform_.rotate, timer_);
		data_.Sort();
		fitViewRequested_ = true;
	}
	if (mode_ == EditorMode::kPlay && data_.totalTime > 0.0f) {
		applyCamera = true;
		timer_ += deltaTime;
		if (loop_) {
			timer_ = std::fmod(timer_, data_.totalTime);
		} else if (timer_ >= data_.totalTime) {
			timer_ = data_.totalTime;
			mode_ = EditorMode::kStop;
		}
	}

	if (applyCamera) {
		ApplyCameraAt(timer_);
	} else {
		CameraGizmo();
	}
}

void CameraEditor::ApplyCameraAt(float time) {
	transform_.translate = {
		data_.posXCurve.Evaluate(time), data_.posYCurve.Evaluate(time), data_.posZCurve.Evaluate(time)
	};
	transform_.rotate = {
		data_.rotXCurve.Evaluate(time), data_.rotYCurve.Evaluate(time), data_.rotZCurve.Evaluate(time)
	};
}

void CameraEditor::CameraGizmo() {
#ifdef USE_IMGUI

	if (!isGizmoActive_) {
		return;
	}

	ImGuizmo::Enable(true);

	static ImGuizmo::OPERATION op = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;

	if (ImGui::IsKeyPressed(ImGuiKey_T)) op = ImGuizmo::OPERATION::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_R)) op = ImGuizmo::OPERATION::ROTATE;

	float view[16];
	float projection[16];
	float world[16];
	Matrix4x4 worldMat = GameEngine::Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

	std::memcpy(view, viewMatrix_.m, sizeof(float) * 16);
	std::memcpy(projection, projectionMatrix_.m, sizeof(float) * 16);
	std::memcpy(world, worldMat.m, sizeof(float) * 16);

	bool different = ImGuizmo::Manipulate(view, projection, op, mode, world);

	float translation[3], rotation[3], scale[3];
	ImGuizmo::DecomposeMatrixToComponents(world, translation, rotation, scale);

	rotation[0] *= std::numbers::pi_v<float> / 180.0f;
	rotation[1] *= std::numbers::pi_v<float> / 180.0f;
	rotation[2] *= std::numbers::pi_v<float> / 180.0f;

	switch (op) {
	case ImGuizmo::TRANSLATE:
		transform_.translate = { translation[0], translation[1], translation[2] };
		break;
	case ImGuizmo::ROTATE:
		transform_.rotate = { rotation[0], rotation[1], rotation[2] };
		break;
	}

#endif
}

void CameraEditor::CurveDraw() {
#ifdef USE_IMGUI
	ImGui::SeparatorText("Curves");
	auto switchGroup = [&](int group) {
		if (curveGroup_ == group) return;
		valueViewCenter_[curveGroup_] = (viewValueMin_ + viewValueMax_) * 0.5f;
		curveGroup_ = group;
		const float halfRange = valuePerGrid_[curveGroup_] * 5.0f;
		viewValueMin_ = valueViewCenter_[curveGroup_] - halfRange;
		viewValueMax_ = valueViewCenter_[curveGroup_] + halfRange;
		if (selectedCurve_ / 3 != curveGroup_) {
			selectedCurve_ = curveGroup_ * 3;
			selectedKeyId_ = UINT32_MAX;
		}
	};
	if (ImGui::RadioButton("Position", curveGroup_ == 0)) switchGroup(0);
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotation", curveGroup_ == 1)) switchGroup(1);

	const int firstCurve = curveGroup_ * 3;
	for (int i = firstCurve; i < firstCurve + 3; ++i) {
		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_Text, kCurveColors[i]);
		ImGui::Checkbox(kCurveNames[i], &curveVisible_[i]);
		ImGui::PopStyleColor();
		if (i != firstCurve + 2) ImGui::SameLine();
		ImGui::PopID();
	}
	if (ImGui::Button("Frame All")) fitViewRequested_ = true;
	ImGui::SameLine();
	for (int group = 0; group < 2; ++group) {
		ImGui::SetNextItemWidth(115.0f);
		float valuePerGrid = valuePerGrid_[group];
		const char* label = group == 0 ? "Position / Grid" : "Rotation / Grid";
		if (ImGui::DragFloat(label, &valuePerGrid, std::max(valuePerGrid_[group] * 0.01f, 0.001f),
			0.001f, 1000000.0f, "%.3f")) {
			valuePerGrid_[group] = std::max(valuePerGrid, 0.001f);
			if (curveGroup_ == group) {
				const float center = (viewValueMin_ + viewValueMax_) * 0.5f;
				const float halfRange = valuePerGrid_[group] * 5.0f;
				viewValueMin_ = center - halfRange;
				viewValueMax_ = center + halfRange;
				valueViewCenter_[group] = center;
			}
		}
		if (group == 0) ImGui::SameLine();
	}
	ImGui::TextDisabled("LMB: drag  Double-click: add  Wheel: zoom  RMB: pan  Delete: remove");

	const ImVec2 canvasSize(std::max(ImGui::GetContentRegionAvail().x, 300.0f),
		std::max(ImGui::GetContentRegionAvail().y, 280.0f));
	const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
	const ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
	ImGui::InvisibleButton("##CameraCurveCanvas", canvasSize,
		ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(24, 26, 31, 255));
	drawList->PushClipRect(canvasMin, canvasMax, true);

	if (fitViewRequested_) {
		float maxTime = std::max(1.0f, data_.totalTime);
		float minValue = FLT_MAX;
		float maxValue = -FLT_MAX;
		for (int curveIndex = firstCurve; curveIndex < firstCurve + 3; ++curveIndex) {
			if (!curveVisible_[curveIndex]) continue;
			for (const CurveKey& key : data_.GetCurve(curveIndex).GetKeys()) {
				minValue = std::min(minValue, std::min(key.value, key.value + key.leftHandle.y));
				maxValue = std::max(maxValue, std::max(key.value, key.value + key.rightHandle.y));
				maxTime = std::max(maxTime, key.time);
			}
		}
		if (minValue == FLT_MAX) {
			minValue = -1.0f;
			maxValue = 1.0f;
		}
		const float padding = std::max((maxValue - minValue) * 0.15f, 0.5f);
		viewTimeMin_ = 0.0f;
		viewTimeMax_ = maxTime + std::max(maxTime * 0.05f, 0.1f);
		viewValueMin_ = minValue - padding;
		viewValueMax_ = maxValue + padding;
		valuePerGrid_[curveGroup_] = (viewValueMax_ - viewValueMin_) / 10.0f;
		valueViewCenter_[curveGroup_] = (viewValueMin_ + viewValueMax_) * 0.5f;
		fitViewRequested_ = false;
	}

	auto toScreen = [&](float time, float value) {
		const float x = (time - viewTimeMin_) / (viewTimeMax_ - viewTimeMin_);
		const float y = (value - viewValueMin_) / (viewValueMax_ - viewValueMin_);
		return ImVec2(canvasMin.x + x * canvasSize.x, canvasMax.y - y * canvasSize.y);
	};
	auto toCurve = [&](ImVec2 screen) {
		const float x = (screen.x - canvasMin.x) / canvasSize.x;
		const float y = (canvasMax.y - screen.y) / canvasSize.y;
		return Vector2{
			viewTimeMin_ + x * (viewTimeMax_ - viewTimeMin_),
			viewValueMin_ + y * (viewValueMax_ - viewValueMin_)
		};

	};

	if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
		const Vector2 focus = toCurve(ImGui::GetIO().MousePos);
		const float factor = std::pow(0.85f, ImGui::GetIO().MouseWheel);
		viewTimeMin_ = focus.x + (viewTimeMin_ - focus.x) * factor;
		viewTimeMax_ = focus.x + (viewTimeMax_ - focus.x) * factor;
		viewValueMin_ = focus.y + (viewValueMin_ - focus.y) * factor;
		viewValueMax_ = focus.y + (viewValueMax_ - focus.y) * factor;
		valuePerGrid_[curveGroup_] = (viewValueMax_ - viewValueMin_) / 10.0f;
		valueViewCenter_[curveGroup_] = (viewValueMin_ + viewValueMax_) * 0.5f;
	}
	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		const float dt = -delta.x / canvasSize.x * (viewTimeMax_ - viewTimeMin_);
		const float dv = delta.y / canvasSize.y * (viewValueMax_ - viewValueMin_);
		viewTimeMin_ += dt; viewTimeMax_ += dt;
		viewValueMin_ += dv; viewValueMax_ += dv;
	}

	for (int i = 0; i <= 10; ++i) {
		const float ratio = i / 10.0f;
		const float x = canvasMin.x + canvasSize.x * ratio;
		const float y = canvasMin.y + canvasSize.y * ratio;
		drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), IM_COL32(55, 58, 66, 150));
		drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(55, 58, 66, 150));
		char label[32];
		sprintf_s(label, "%.2f", viewTimeMin_ + ratio * (viewTimeMax_ - viewTimeMin_));
		drawList->AddText(ImVec2(x + 3.0f, canvasMax.y - 18.0f), IM_COL32(170, 175, 185, 255), label);
		sprintf_s(label, "%.2f", viewValueMax_ - ratio * (viewValueMax_ - viewValueMin_));
		drawList->AddText(ImVec2(canvasMin.x + 4.0f, y + 2.0f), IM_COL32(170, 175, 185, 255), label);
	}
	if (viewValueMin_ <= 0.0f && viewValueMax_ >= 0.0f) {
		const float y = toScreen(0.0f, 0.0f).y;
		drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), IM_COL32(120, 125, 135, 220), 1.5f);
	}

	float nearestDistance = 100.0f;
	int hoveredCurve = -1;
	uint32_t hoveredKey = UINT32_MAX;
	for (int curveIndex = firstCurve; curveIndex < firstCurve + 3; ++curveIndex) {
		if (!curveVisible_[curveIndex]) continue;
		CameraAnimationCurve& curve = data_.GetCurve(curveIndex);
		const auto& keys = curve.GetKeys();
		for (size_t segment = 0; segment + 1 < keys.size(); ++segment) {
			const CurveKey& a = keys[segment];
			const CurveKey& b = keys[segment + 1];
			ImVec2 previous = toScreen(a.time, a.value);
			for (int sample = 1; sample <= 48; ++sample) {
				const float time = a.time + (b.time - a.time) * (sample / 48.0f);
				const ImVec2 current = toScreen(time, curve.Evaluate(time));
				drawList->AddLine(previous, current, kCurveColors[curveIndex], 2.0f);
				previous = current;
			}
		}
		for (const CurveKey& key : keys) {
			const ImVec2 point = toScreen(key.time, key.value);
			const bool selected = selectedCurve_ == curveIndex && selectedKeyId_ == key.id;
			drawList->AddCircleFilled(point, selected ? 6.0f : 4.0f,
				selected ? IM_COL32_WHITE : kCurveColors[curveIndex]);
			const float distance = DistanceSquared(point, ImGui::GetIO().MousePos);
			if (hovered && distance < nearestDistance) {
				nearestDistance = distance;
				hoveredCurve = curveIndex;
				hoveredKey = key.id;
			}
		}
	}

	CurveKey* selectedKey = nullptr;
	if (selectedKeyId_ != UINT32_MAX) {
		for (CurveKey& key : data_.GetCurve(selectedCurve_).GetKeys()) {
			if (key.id == selectedKeyId_) { selectedKey = &key; break; }
		}
	}
	int hoveredHandle = 0;
	if (selectedKey) {
		const ImVec2 keyPoint = toScreen(selectedKey->time, selectedKey->value);
		const ImVec2 leftPoint = toScreen(selectedKey->time + selectedKey->leftHandle.x,
			selectedKey->value + selectedKey->leftHandle.y);
		const ImVec2 rightPoint = toScreen(selectedKey->time + selectedKey->rightHandle.x,
			selectedKey->value + selectedKey->rightHandle.y);
		drawList->AddLine(keyPoint, leftPoint, IM_COL32(190, 190, 195, 255));
		drawList->AddLine(keyPoint, rightPoint, IM_COL32(190, 190, 195, 255));
		drawList->AddCircleFilled(leftPoint, 4.5f, IM_COL32(235, 235, 240, 255));
		drawList->AddCircleFilled(rightPoint, 4.5f, IM_COL32(235, 235, 240, 255));
		if (hovered && DistanceSquared(leftPoint, ImGui::GetIO().MousePos) < 81.0f) hoveredHandle = -1;
		if (hovered && DistanceSquared(rightPoint, ImGui::GetIO().MousePos) < 81.0f) hoveredHandle = 1;
	}

	if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		const Vector2 point = toCurve(ImGui::GetIO().MousePos);
		CurveKey key;
		key.time = std::max(0.0f, point.x);
		key.value = point.y;
		const float length = std::max((viewTimeMax_ - viewTimeMin_) * 0.04f, 0.02f);
		key.leftHandle = { -length, 0.0f };
		key.rightHandle = { length, 0.0f };
		data_.GetCurve(selectedCurve_).AddKey(key);
		data_.totalTime = std::max(data_.totalTime, key.time);
	} else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (hoveredHandle != 0 && selectedKey) {
			dragTarget_ = hoveredHandle < 0 ? DragTarget::kLeftHandle : DragTarget::kRightHandle;
		} else if (hoveredKey != UINT32_MAX) {
			selectedCurve_ = hoveredCurve;
			selectedKeyId_ = hoveredKey;
			dragTarget_ = DragTarget::kKey;
		} else {
			selectedKeyId_ = UINT32_MAX;
			dragTarget_ = DragTarget::kNone;
		}
	}

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && selectedKey) {
		const Vector2 cursor = toCurve(ImGui::GetIO().MousePos);
		if (dragTarget_ == DragTarget::kKey) {
			selectedKey->time = std::max(0.0f, cursor.x);
			selectedKey->value = cursor.y;
		} else if (dragTarget_ == DragTarget::kLeftHandle) {
			selectedKey->leftHandle = {
				std::min(0.0f, cursor.x - selectedKey->time), cursor.y - selectedKey->value
			};
		} else if (dragTarget_ == DragTarget::kRightHandle) {
			selectedKey->rightHandle = {
				std::max(0.0f, cursor.x - selectedKey->time), cursor.y - selectedKey->value
			};
		}
	}
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && dragTarget_ != DragTarget::kNone) {
		data_.GetCurve(selectedCurve_).SortKeys();
		data_.Sort();
		dragTarget_ = DragTarget::kNone;
	}
	if (selectedKeyId_ != UINT32_MAX && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		data_.GetCurve(selectedCurve_).RemoveKey(selectedKeyId_);
		selectedKeyId_ = UINT32_MAX;
		data_.Sort();
	}

	drawList->AddLine(toScreen(timer_, viewValueMin_), toScreen(timer_, viewValueMax_),
		IM_COL32(255, 220, 75, 255), 2.0f);
	drawList->PopClipRect();
#endif
}
