#pragma once
#include <array>
#include <string>
#include "DebugParameter.h"
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

	ScoreView(
		const DigitModels& models,
		const GameEngine::Camera* camera,
		const std::string& parameterGroupName = "ScoreView");

	void SetValue(int value);
	void Update();
	void Draw(GameEngine::RenderQueue* renderQueue);

	void SetPosition(const Vector3& position) { position_ = position; }
	void SetScale(float scale) { scale_ = scale; }
	void SetDigitSpacing(float spacing) { digitSpacing_ = spacing; }
	void SetColor(const Vector4& color) { color_ = color; }

private:
	DigitModels models_;
	const GameEngine::Camera* camera_ = nullptr;
	std::array<GameEngine::WorldTransform, kDigitCount> digitTransforms_;
	std::array<int, kDigitCount> digits_{};

	// 左から万・千・百・十・一の位、Translateは共通配置からの追加移動量
	std::array<Vector3, kDigitCount> digitTranslations_{};
	std::array<Vector3, kDigitCount> digitRotations_{};
	GameEngine::Material material_;

	// カメラ座標での左端の数字の位置・サイズ・桁間隔
	Vector3 position_ = { -3.2f, 1.8f, 10.0f }; // 左端の数字の位置
	float scale_ = 0.15f;                       // サイズ
	float digitSpacing_ = 0.45f;                // 桁間隔
	bool hideLeadingZeros_ = true;              // 先頭の0を非表示にするかどうか
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 全桁へまとめて適用する色
	GameEngine::DebugParameter debugParameter_;
};
