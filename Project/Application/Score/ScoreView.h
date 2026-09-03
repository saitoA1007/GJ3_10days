#pragma once
#include <array>
#include "Material.h"
#include "WorldTransform.h"

namespace GameEngine {
	class Camera;
	class Model;
	class RenderQueue;
}

// スコアの表示値を受け取り、数字モデルを5桁分描画する。
class ScoreView {
public:
	static constexpr int kDigitCount = 5;
	using DigitModels = std::array<GameEngine::Model*, 10>;

	ScoreView(const DigitModels& models, const GameEngine::Camera* camera);

	void SetValue(int value);
	void Draw(GameEngine::RenderQueue* renderQueue);
	void DebugUpdate();

	void SetPosition(const Vector3& position) { position_ = position; }
	void SetScale(float scale) { scale_ = scale; }
	void SetDigitSpacing(float spacing) { digitSpacing_ = spacing; }

private:
	DigitModels models_;
	const GameEngine::Camera* camera_ = nullptr;
	std::array<GameEngine::WorldTransform, kDigitCount> digitTransforms_;
	std::array<int, kDigitCount> digits_{};
	GameEngine::Material material_;
	int displayedValue_ = 0;

	// カメラ座標での左端の数字の位置・サイズ・桁間隔。
	Vector3 position_ = { -3.2f, 1.8f, 10.0f };
	float scale_ = 0.15f;
	float digitSpacing_ = 0.45f;
};
