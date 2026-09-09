#include "ScoreView.h"

#include <algorithm>
#include <cassert>
#include <string>
#include "Camera.h"
#include "Model.h"
#include "MyMath.h"
#include "RenderQueue.h"

ScoreView::ScoreView(
	const DigitModels& models,
	const GameEngine::Camera* camera,
	const std::string& parameterGroupName,
	const Settings& settings)
	: models_(models), camera_(camera), debugParameter_(parameterGroupName) {
	assert(camera_);
	for (const auto* model : models_) {
		assert(model && "Number models 0.obj through 9.obj must be loaded.");
	}
	digitCount_ = std::clamp(settings.digitCount, 1, kMaxDigitCount);
	hideLeadingZeros_ = settings.hideLeadingZeros;
	// 桁数を後から増やしても向きが崩れないよう、未使用の桁も既定の向きで初期化する。
	digitRotations_.fill(settings.digitRotation);

	material_.Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, false);
	// 配置を外側のUIが決める場合は、効かないRegisterをInspectorへ出さない。
	if (settings.registerLayoutParameters) {
		debugParameter_.Register("Position", position_, 0);
		debugParameter_.Register("Scale", scale_, 1);
		debugParameter_.Register("DigitSpacing", digitSpacing_, 2);
		debugParameter_.Register("HideLeadingZeros", hideLeadingZeros_, 3);
		debugParameter_.Register("Color", color_, 4);
	}

	for (int i = 0; i < digitCount_; ++i) {
		const std::string group = "Digit" + std::to_string(digitCount_ - i);
		debugParameter_.Register("Translate", digitTranslations_[i], 0, group);
		debugParameter_.Register("Rotate", digitRotations_[i], 1, group);
	}
	debugParameter_.Apply();
	material_.SetColor(color_);
	SetValue(0);
}

void ScoreView::SetValue(int value) {
	value_ = value;

	// 桁数に収まる最大値へ丸めてから、下の位から順に分解する。
	int maxValue = 1;
	for (int i = 0; i < digitCount_; ++i) {
		maxValue *= 10;
	}
	int remaining = std::clamp(value, 0, maxValue - 1);
	for (int i = digitCount_ - 1; i >= 0; --i) {
		digits_[i] = remaining % 10;
		remaining /= 10;
	}
}

void ScoreView::SetDigitCount(int digitCount) {
	const int clampedDigitCount = std::clamp(digitCount, 1, kMaxDigitCount);
	if (clampedDigitCount == digitCount_) {
		return;
	}
	digitCount_ = clampedDigitCount;
	SetValue(value_);
}

void ScoreView::Update() {
	debugParameter_.ApplyIfDirty();
	material_.SetColor(color_);
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
	for (int i = 0; i < digitCount_; ++i) {
		// 先頭の0だけを省略する、スコア0でも一の位は必ず表示する
		if (leadingZero && digits_[i] == 0 && i < digitCount_ - 1) {
			continue;
		}
		leadingZero = false;
		const auto* model = models_[digits_[i]];
		if (!model) {
			continue;
		}
		const Vector3 position = position_ + Vector3{ digitSpacing_ * static_cast<float>(i), 0.0f, 0.0f } + digitTranslations_[i];
		digitTransforms_[i].UpdateWorldMatrix(GameEngine::Math::MakeAffineMatrix(scale, digitRotations_[i], position) * cameraWorld);
		renderQueue->SubmitModel(model, digitTransforms_[i], color_.w, &material_.GetMaterialBuffer());
	}
}
