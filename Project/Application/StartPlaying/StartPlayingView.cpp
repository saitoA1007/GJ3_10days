#include "StartPlayingView.h"

#include <cassert>
#include <string>

#include "Camera.h"
#include "ModelComponent.h"
#include "MyMath.h"
#include "RenderQueue.h"

namespace
{
	Matrix4x4 MakeCameraFacingWorldMatrix(
		const Transform& transform,
		const Matrix4x4& cameraWorld)
	{
		// SRTのTranslateは画面上の配置としてカメラローカル座標のまま扱う。
		const Vector3 worldPosition =
			GameEngine::Math::Transforms(transform.translate, cameraWorld);

		// カメラの回転だけをビルボードへ適用し、位置は上で求めた座標へ設定する。
		Matrix4x4 billboardRotation = cameraWorld;
		billboardRotation.m[3][0] = 0.0f;
		billboardRotation.m[3][1] = 0.0f;
		billboardRotation.m[3][2] = 0.0f;

		return GameEngine::Math::MakeAffineMatrix(
			transform.scale,
			transform.rotate,
			{}) *
			billboardRotation *
			GameEngine::Math::MakeTranslateMatrix(worldPosition);
	}
}

StartPlayingView::StartPlayingView(
	const Models& models,
	const GameEngine::Camera* camera)
	: camera_(camera)
{
	constexpr Vector3 kCenterPosition = { 0.0f, 0.0f, 10.0f };
	constexpr Vector3 kRotation = { 1.57079637f, 3.14159274f, 0.0f };
	constexpr float kScale = 0.6f;
	constexpr float kSpacing = 0.55f;
	const float centerIndex = static_cast<float>(kModelCount - 1) * 0.5f;

	assert(camera_ && "StartPlayingView requires a camera.");
	for (std::size_t i = 0; i < models_.size(); ++i)
	{
		assert(models[i] && "playStartLogo0.obj through playStartLogo12.obj must be loaded.");
		if (!models[i])
		{
			continue;
		}

		models_[i] = std::make_unique<GameEngine::ModelComponent>(models[i]);
		models_[i]->SetEnableLighting(false);

		Transform& transform = models_[i]->worldTransform_.transform_;
		transform.scale = { kScale, kScale, kScale };
		transform.rotate = kRotation;
		transform.translate = kCenterPosition + Vector3{
			(static_cast<float>(i) - centerIndex) * kSpacing,
			0.0f,
			0.0f };

		const std::string groupName = "PlayStartLogo" + std::to_string(i);
		debugParameter_.Register("Scale", transform.scale, 0, groupName);
		debugParameter_.Register("Rotate", transform.rotate, 1, groupName);
		debugParameter_.Register("Translate", transform.translate, 2, groupName);
	}

	debugParameter_.Apply();
}

StartPlayingView::~StartPlayingView() = default;

void StartPlayingView::Start()
{
	isVisible_ = true;
}

void StartPlayingView::Update(float deltaTime)
{
	(void)deltaTime;
	debugParameter_.ApplyIfDirty();
}

void StartPlayingView::Stop()
{
	isVisible_ = false;
}

void StartPlayingView::Reset()
{
	isVisible_ = false;
}

void StartPlayingView::Draw(GameEngine::RenderQueue* renderQueue)
{
	if (!isVisible_ || !camera_ || !renderQueue)
	{
		return;
	}

	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();

	for (std::size_t i = 0; i < models_.size(); ++i)
	{
		if (!models_[i])
		{
			continue;
		}

		const Transform& transform =
			models_[i]->worldTransform_.transform_;
		models_[i]->worldTransform_.UpdateWorldMatrix(
			MakeCameraFacingWorldMatrix(transform, cameraWorld));
		models_[i]->Draw(renderQueue);
	}
}
