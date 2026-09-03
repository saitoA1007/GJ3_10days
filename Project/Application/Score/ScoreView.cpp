#include "ScoreView.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include "Camera.h"
#include "ImGuiManager.h"
#include "Model.h"
#include "MyMath.h"
#include "RenderQueue.h"

ScoreView::ScoreView(const DigitModels& models, const GameEngine::Camera* camera)
	: models_(models), camera_(camera) {
	assert(camera_);
	for (const auto* model : models_) {
		assert(model && "Number models 0.obj through 9.obj must be loaded.");
	}
	material_.Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, false);
	SetValue(0);
}

void ScoreView::SetValue(int value) {
	displayedValue_ = std::clamp(value, 0, 99999);
	int remaining = displayedValue_;
	for (int i = kDigitCount - 1; i >= 0; --i) {
		digits_[i] = remaining % 10;
		remaining /= 10;
	}
}

void ScoreView::Draw(GameEngine::RenderQueue* renderQueue) {
	if (!camera_) {
		return;
	}

	// 描画に使うカメラへ追従させ、カメラが移動しても同じ場所に表示する。
	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();
	// NumberのモデルはXZ平面上にある。読み込み時のX反転も含め、正面向きに回転する。
	const Vector3 rotation = { std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float>, 0.0f };
	const Vector3 scale = { scale_, scale_, scale_ };
	for (int i = 0; i < kDigitCount; ++i) {
		const auto* model = models_[digits_[i]];
		if (!model) {
			continue;
		}
		const Vector3 position = position_ + Vector3{ digitSpacing_ * static_cast<float>(i), 0.0f, 0.0f };
		digitTransforms_[i].UpdateWorldMatrix(GameEngine::Math::MakeAffineMatrix(scale, rotation, position) * cameraWorld);
		renderQueue->SubmitModel(model, digitTransforms_[i], 1.0f, &material_.GetMaterialBuffer());
	}
}

void ScoreView::DebugUpdate() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Score", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Score: %05d", displayedValue_);
		ImGui::DragFloat3("Position (camera)", &position_.x, 0.05f);
		ImGui::DragFloat("Scale", &scale_, 0.005f, 0.001f, 5.0f);
		ImGui::DragFloat("Digit spacing", &digitSpacing_, 0.01f, 0.0f, 10.0f);
	}
	ImGui::End();
#endif
}
