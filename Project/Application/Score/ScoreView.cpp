#include "ScoreView.h"

#include <algorithm>
#include <cassert>
#include <numbers>
#include <string>
#include "Camera.h"
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
	debugParameter_.Register("Position", position_, 0);
	debugParameter_.Register("Scale", scale_, 1);
	debugParameter_.Register("DigitSpacing", digitSpacing_, 2);
	debugParameter_.Register("HideLeadingZeros", hideLeadingZeros_, 3);

	for (int i = 0; i < kDigitCount; ++i) {
		// NumberのXZ平面と読み込み時のX反転を補正した向きを初期値にする。
		digitRotations_[i] = { std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float>, 0.0f };
		const std::string group = "Digit" + std::to_string(kDigitCount - i);
		debugParameter_.Register("Translate", digitTranslations_[i], 0, group);
		debugParameter_.Register("Rotate", digitRotations_[i], 1, group);
	}
	debugParameter_.Apply();
	SetValue(0);
}

void ScoreView::SetValue(int value) {
	int remaining = std::clamp(value, 0, 99999);
	for (int i = kDigitCount - 1; i >= 0; --i) {
		digits_[i] = remaining % 10;
		remaining /= 10;
	}
}

void ScoreView::Update() {
	debugParameter_.ApplyIfDirty();
}

void ScoreView::Draw(GameEngine::RenderQueue* renderQueue) {
	if (!camera_) {
		return;
	}

	// 描画に使うカメラへ追従させ、カメラが移動しても同じ場所に表示する
	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();
	const Vector3 scale = { scale_, scale_, scale_ };
	bool leadingZero = hideLeadingZeros_;
	for (int i = 0; i < kDigitCount; ++i) {
		// 先頭の0だけを省略する、スコア0でも一の位は必ず表示する
		if (leadingZero && digits_[i] == 0 && i < kDigitCount - 1) {
			continue;
		}
		leadingZero = false;
		const auto* model = models_[digits_[i]];
		if (!model) {
			continue;
		}
		const Vector3 position = position_ + Vector3{ digitSpacing_ * static_cast<float>(i), 0.0f, 0.0f } + digitTranslations_[i];
		digitTransforms_[i].UpdateWorldMatrix(GameEngine::Math::MakeAffineMatrix(scale, digitRotations_[i], position) * cameraWorld);
		renderQueue->SubmitModel(model, digitTransforms_[i], 1.0f, &material_.GetMaterialBuffer());
	}
}
