#pragma once
#include <array>
#include <numbers>
#include <string>
#include "DebugParameter.h"
#include "Material.h"
#include "WorldTransform.h"

namespace GameEngine {
	class Camera;
	class Model;
	class RenderQueue;
}

// 表示値を受け取り、数字モデルを指定された桁数分描画する。
class ScoreView {
public:
	static constexpr int kMaxDigitCount = 5;
	using DigitModels = std::array<GameEngine::Model*, 10>;

	// 桁数や向き、Registerへ公開する項目を切り替えるための生成設定。
	struct Settings {
		int digitCount = kMaxDigitCount;  // 実際に描画する桁数(1～kMaxDigitCount)
		bool hideLeadingZeros = true;     // 先頭の0を省略するかどうか
		// NumberのXZ平面と読み込み時のX反転を補正した向き
		Vector3 digitRotation = { std::numbers::pi_v<float> *0.5f, std::numbers::pi_v<float>, 0.0f };
		// 位置・サイズ・桁間隔・色をRegisterへ公開するか。外側から配置を決める場合はfalseにする
		bool registerLayoutParameters = true;
	};

	ScoreView(
		const DigitModels& models,
		const GameEngine::Camera* camera,
		const std::string& parameterGroupName = "ScoreView",
		const Settings& settings = Settings{});

	void SetValue(int value);
	void Update();
	void Draw(GameEngine::RenderQueue* renderQueue);

	/// @brief 描画する桁数を変更する。表示中の値は新しい桁数で分解し直す。
	/// @param[in] digitCount 1～kMaxDigitCountへ丸められる桁数。
	void SetDigitCount(int digitCount);

	void SetPosition(const Vector3& position) { position_ = position; }
	void SetScale(float scale) { scale_ = scale; }
	void SetDigitSpacing(float spacing) { digitSpacing_ = spacing; }
	void SetColor(const Vector4& color) { color_ = color; }

	/// @brief 現在の桁数を取得する。
	int GetDigitCount() const { return digitCount_; }

	/// @brief 左端から次の文字を置くまでに必要な幅を取得する。
	float GetWidth() const { return digitSpacing_ * static_cast<float>(digitCount_); }

private:
	DigitModels models_;
	const GameEngine::Camera* camera_ = nullptr;
	std::array<GameEngine::WorldTransform, kMaxDigitCount> digitTransforms_;
	std::array<int, kMaxDigitCount> digits_{};

	// 左から上位の位、Translateは共通配置からの追加移動量
	std::array<Vector3, kMaxDigitCount> digitTranslations_{};
	std::array<Vector3, kMaxDigitCount> digitRotations_{};
	GameEngine::Material material_;

	// カメラ座標での左端の数字の位置・サイズ・桁間隔
	Vector3 position_ = { -3.2f, 1.8f, 10.0f }; // 左端の数字の位置
	float scale_ = 0.15f;                       // サイズ
	float digitSpacing_ = 0.45f;                // 桁間隔
	bool hideLeadingZeros_ = true;              // 先頭の0を非表示にするかどうか
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 全桁へまとめて適用する色
	int digitCount_ = kMaxDigitCount;            // 実際に描画する桁数
	int value_ = 0;                              // 桁数変更時に分解し直すための保持値
	GameEngine::DebugParameter debugParameter_;
};
