#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include "DebugParameter.h"
#include "../Utils/GameTimer.h"

namespace GameEngine {
	class ModelComponent;
	class ModelManager;
	class RenderQueue;
}

/// <summary>
/// タイトルロゴを構成するモデルをまとめて管理する。
/// </summary>
class TitleLogo final {
public:
	explicit TitleLogo(GameEngine::ModelManager* modelManager);
	~TitleLogo();

	void Update();
	void Draw(GameEngine::RenderQueue* renderQueue);

	void AnimationStart() { if (!isAnimating_) { isAnimating_ = true; animationTimer_.Start(1.0f, false); } }

private:
	static constexpr std::size_t kPartCount = 4;
	std::unique_ptr<GameEngine::ModelComponent> bottom_;
	std::array<std::unique_ptr<GameEngine::ModelComponent>, kPartCount> parts_;
	GameEngine::DebugParameter debugParameter_{ "TitleLogo" };

	bool isAnimating_ = true;
	GameTimer animationTimer_;
};
