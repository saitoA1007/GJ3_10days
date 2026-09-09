#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"

#include "Application/Score/ScoreView.h"

namespace GameEngine
{
	class Camera;
}

class UnitManager;

/// @brief 数字モデルでユニットの数を表示する。表示内容(現在使用出来るユニットの数/ユニットの総数)
class UnitCountUI final : public GameEngine::IGameObject
{
public:
	/// @brief 0～9のモデルとカメラを使い、ユニット数を[00/00]形式で表示する。
	/// @param[in] digitModels 0から9までの数字モデル。
	/// @param[in] camera 画面固定配置に使うカメラ。
	/// @param[in] unitManager 表示するユニット数の取得元。
	UnitCountUI(
		const ScoreView::DigitModels& digitModels,
		const GameEngine::Camera* camera,
		const UnitManager* unitManager);
	~UnitCountUI() override = default;

	/// @brief 初期のユニット数を数字表示へ反映する。
	void Initialize() override;

	/// @brief 現在のユニット数とRegister設定を反映する。
	void Update() override;

	/// @brief 停止中も現在値とRegister設定を反映する。
	void DebugUpdate() override;

	/// @brief 数字モデルを描画キューへ登録する。
	void Draw() override;

private:
	/// @brief 最新のユニット数を取り込み、桁数と配置を決め直す。
	void SyncValue();

	/// @brief Register値を安全な範囲へ補正する。
	void SanitizeSettings();

	/// @brief 値を表示するのに必要な桁数を求める。
	/// @param[in] value 桁数を数えたい値。
	/// @return 1以上kMaxDigitCount以下の桁数。
	static int CountDigits(int value);

	const UnitManager* unitManager_ = nullptr;                 // 表示するユニット数の取得元
	GameEngine::DebugParameter debugParameter_{ "UnitCountUI" };// 配置調整用Register

	// カメラ座標での左端の位置と、3つのViewで共有する見た目
	Vector3 position_ = { -3.2f, 1.2f, 10.0f }; // 左端の数字の位置
	float scale_ = 0.15f;                       // 数字のサイズ
	float digitSpacing_ = 0.45f;                // 桁間隔
	float separatorGap_ = 0.05f;                // 区切り記号の前後へ追加する間隔
	float separatorScaleRate_ = 1.0f;           // 数字に対する区切り記号のサイズ比
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };// 全体へまとめて適用する色

	ScoreView availableView_; // 現在使用出来るユニット数(左側)
	ScoreView separatorView_; // 1.objを傾けて代用する区切り記号"/"
	ScoreView totalView_;     // ユニットの総数(右側)
};
