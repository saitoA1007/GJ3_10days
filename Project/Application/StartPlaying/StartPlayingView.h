#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "DebugParameter.h"

namespace GameEngine
{
	class Camera;
	class Model;
	class ModelComponent;
	class RenderQueue;
}

/// @brief StartPlaying中に表示するロゴモデルをまとめて管理する。
class StartPlayingView final
{
public:
	static constexpr std::size_t kModelCount = 13;
	using Models = std::array<GameEngine::Model*, kModelCount>;

	StartPlayingView(
		const Models& models,
		const GameEngine::Camera* camera);
	~StartPlayingView();

	void Start();
	void Update(float deltaTime);
	void Stop();
	void Reset();
	void Draw(GameEngine::RenderQueue* renderQueue);

private:
	const GameEngine::Camera* camera_ = nullptr;
	std::array<std::unique_ptr<GameEngine::ModelComponent>, kModelCount> models_;
	GameEngine::DebugParameter debugParameter_{ "StartPlayingView" };
	bool isVisible_ = false;
};
