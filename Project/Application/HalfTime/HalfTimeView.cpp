#include "HalfTimeView.h"

#include <algorithm>
#include <cassert>

#include "Camera.h"
#include "ModelComponent.h"
#include "MyMath.h"
#include "RenderQueue.h"

HalfTimeView::HalfTimeView(
	GameEngine::Model* model,
	const GameEngine::Camera* camera,
	const Settings& defaults)
	: camera_(camera), settings_(defaults)
{
	assert(model && "HalfTimeView requires harfTime.obj.");
	assert(camera_ && "HalfTimeView requires a camera.");
	if (model)
	{
		model_ = std::make_unique<GameEngine::ModelComponent>(model);
		model_->SetEnableLighting(false);
	}

	RegisterTransformParameters("Start", settings_.start);
	RegisterTransformParameters("Hold", settings_.hold);
	RegisterTransformParameters("End", settings_.end);
	debugParameter_.Register("MoveInDuration", settings_.moveInDuration, 0, "Time");
	debugParameter_.Register("HoldDuration", settings_.holdDuration, 1, "Time");
	debugParameter_.Register("MoveOutDuration", settings_.moveOutDuration, 2, "Time");
	debugParameter_.Register("MoveInEaseType", settings_.moveInEaseType, 0, "Easing");
	debugParameter_.Register("MoveOutEaseType", settings_.moveOutEaseType, 1, "Easing");
	debugParameter_.Apply();
	SanitizeSettings();
	Reset();
}

HalfTimeView::~HalfTimeView() = default;

void HalfTimeView::Start()
{
	debugParameter_.ApplyIfDirty();
	SanitizeSettings();
	animationElapsedTime_ = 0.0f;
	isVisible_ = true;
	UpdateCurrentTransform();
}

void HalfTimeView::Update(float deltaTime)
{
	debugParameter_.ApplyIfDirty();
	SanitizeSettings();
	if (!isVisible_)
	{
		return;
	}

	const float totalDuration =
		settings_.moveInDuration +
		settings_.holdDuration +
		settings_.moveOutDuration;
	animationElapsedTime_ = (std::min)(
		animationElapsedTime_ + (std::max)(deltaTime, 0.0f),
		totalDuration);
	UpdateCurrentTransform();
	if (animationElapsedTime_ >= totalDuration)
	{
		isVisible_ = false;
	}
}

void HalfTimeView::Reset()
{
	animationElapsedTime_ = 0.0f;
	isVisible_ = false;
	SetCurrentTransform(settings_.start);
}

void HalfTimeView::Stop()
{
	isVisible_ = false;
}

void HalfTimeView::Draw(GameEngine::RenderQueue* renderQueue)
{
	if (!isVisible_ || !model_ || !camera_ || !renderQueue)
	{
		return;
	}

	const Matrix4x4 cameraWorld = renderQueue->GetUseDebugCamera()
		? renderQueue->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();
	model_->worldTransform_.UpdateWorldMatrix(
		GameEngine::Math::MakeAffineMatrix(
			current_.scale,
			current_.rotation,
			current_.position) * cameraWorld);
	model_->Draw(renderQueue);
}

void HalfTimeView::RegisterTransformParameters(
	const char* groupName,
	TransformSettings& transform)
{
	debugParameter_.Register("Position", transform.position, 0, groupName);
	debugParameter_.Register("Rotate", transform.rotation, 1, groupName);
	debugParameter_.Register("Scale", transform.scale, 2, groupName);
}

void HalfTimeView::SanitizeSettings()
{
	settings_.moveInDuration = (std::max)(settings_.moveInDuration, 0.0f);
	settings_.holdDuration = (std::max)(settings_.holdDuration, 0.0f);
	settings_.moveOutDuration = (std::max)(settings_.moveOutDuration, 0.0f);

	TransformSettings* transforms[] = {
		&settings_.start,
		&settings_.hold,
		&settings_.end,
	};
	for (TransformSettings* transform : transforms)
	{
		transform->scale.x = (std::max)(transform->scale.x, 0.0f);
		transform->scale.y = (std::max)(transform->scale.y, 0.0f);
		transform->scale.z = (std::max)(transform->scale.z, 0.0f);
	}
}

void HalfTimeView::UpdateCurrentTransform()
{
	const float moveInEnd = settings_.moveInDuration;
	const float holdEnd = moveInEnd + settings_.holdDuration;

	if (animationElapsedTime_ < moveInEnd)
	{
		const float progress = settings_.moveInDuration > 0.0f
			? animationElapsedTime_ / settings_.moveInDuration
			: 1.0f;
		InterpolateCurrentTransform(
			settings_.start,
			settings_.hold,
			progress,
			settings_.moveInEaseType);
		return;
	}

	if (animationElapsedTime_ < holdEnd)
	{
		SetCurrentTransform(settings_.hold);
		return;
	}

	const float moveOutElapsed = animationElapsedTime_ - holdEnd;
	const float progress = settings_.moveOutDuration > 0.0f
		? (std::clamp)(moveOutElapsed / settings_.moveOutDuration, 0.0f, 1.0f)
		: 1.0f;
	InterpolateCurrentTransform(
		settings_.hold,
		settings_.end,
		progress,
		settings_.moveOutEaseType);
}

void HalfTimeView::SetCurrentTransform(const TransformSettings& transform)
{
	current_ = transform;
}

void HalfTimeView::InterpolateCurrentTransform(
	const TransformSettings& start,
	const TransformSettings& end,
	float progress,
	EaseType easeType)
{
	current_.position = GameEngine::Lerp(
		start.position,
		end.position,
		progress,
		easeType);
	current_.rotation = GameEngine::Lerp(
		start.rotation,
		end.rotation,
		progress,
		easeType);
	current_.scale = GameEngine::Lerp(
		start.scale,
		end.scale,
		progress,
		easeType);
}
