#pragma once

#include <memory>

#include "DebugParameter.h"
#include "EasingManager.h"
#include "Vector3.h"

namespace GameEngine
{
	class Camera;
	class Model;
	class ModelComponent;
	class RenderQueue;
}

/// @brief Playingが半分を過ぎたときに表示するハーフタイムモデルの演出。
class HalfTimeView final
{
public:
	struct TransformSettings
	{
		Vector3 position = {};
		Vector3 rotation = { 2.01099992f, 3.14159274f, 0.0f };
		Vector3 scale = { 1.0f, 1.0f, 1.0f };
	};

	struct Settings
	{
		TransformSettings start = {
			{ 16.0f, -29.0f, -26.0f },
			{ 2.01099992f, 3.14159274f, 0.0f },
			{ 1.0f, 1.0f, 1.0f } };
		TransformSettings hold = {
			{ 0.0f, -29.0f, -26.0f },
			{ 2.01099992f, 3.14159274f, 0.0f },
			{ 1.0f, 1.0f, 1.0f } };
		TransformSettings end = {
			{ -16.0f, -29.0f, -26.0f },
			{ 2.01099992f, 3.14159274f, 0.0f },
			{ 1.0f, 1.0f, 1.0f } };
		float moveInDuration = 0.5f;
		float holdDuration = 1.0f;
		float moveOutDuration = 0.5f;
		EaseType moveInEaseType = EaseType::kEaseOutCubic;
		EaseType moveOutEaseType = EaseType::kEaseInCubic;
	};

	HalfTimeView(
		GameEngine::Model* model,
		const GameEngine::Camera* camera,
		const Settings& defaults = {});
	~HalfTimeView();

	/// @brief A地点から演出を再生する。
	void Start();
	/// @brief 現在の移動区間を経過時間だけ進める。
	void Update(float deltaTime);
	/// @brief 非表示にして演出を初期状態へ戻す。
	void Reset();
	/// @brief 非表示にする。
	void Stop();
	/// @brief 現在のSRTでモデルを描画する。
	void Draw(GameEngine::RenderQueue* renderQueue);
	/// @brief 演出モデルが表示中か取得する。
	bool IsVisible() const { return isVisible_; }

private:
	void RegisterTransformParameters(
		const char* groupName,
		TransformSettings& transform);
	void SanitizeSettings();
	void UpdateCurrentTransform();
	void SetCurrentTransform(const TransformSettings& transform);
	void InterpolateCurrentTransform(
		const TransformSettings& start,
		const TransformSettings& end,
		float progress,
		EaseType easeType);

	const GameEngine::Camera* camera_ = nullptr;
	std::unique_ptr<GameEngine::ModelComponent> model_;
	Settings settings_;
	TransformSettings current_;
	GameEngine::DebugParameter debugParameter_{ "HalfTimeView" };
	float animationElapsedTime_ = 0.0f;
	bool isVisible_ = false;
};
