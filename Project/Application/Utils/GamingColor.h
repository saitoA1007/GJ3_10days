#pragma once

#include <cstddef>

#include "Vector3.h"

namespace GameEngine {
	class ModelComponent;
}

/// @brief ModelComponentを時間経過で虹色に変化させる汎用機能。
class GamingColor final {
public:
	struct Settings {
		float cycleSpeed = 0.2f;
		float saturation = 1.0f;
		float brightness = 1.0f;
		float colorSpacing = 0.12f;
		bool selfIlluminated = true;
	};

	/// @brief 色相を時間経過させる。
	void Update(float deltaTime);

	/// @brief 色相を指定した位置へ戻す。
	void Reset(float hue = 0.0f);

	/// @brief 現在の虹色をモデルの全マテリアルへ適用する。
	/// @param colorIndex 複数モデルを並べるときの色番号。
	void Apply(GameEngine::ModelComponent& model, std::size_t colorIndex = 0) const;

	/// @brief 指定した色番号の現在色を取得する。
	Vector3 GetColor(std::size_t colorIndex = 0) const;

	Settings& GetSettings() { return settings_; }
	const Settings& GetSettings() const { return settings_; }

private:
	static float NormalizeHue(float hue);

	Settings settings_{};
	float hue_ = 0.0f;
};
