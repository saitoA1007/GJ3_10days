#pragma once

#include "IGameObject.h"

#include "Application/Score/ScoreView.h"

namespace GameEngine {
	class Camera;
}

namespace Prototype {
	class Rocket;

	/// @brief 数字モデルを使ってロケットの現在エネルギーを画面へ表示する。
	class EnergyView final : public GameEngine::IGameObject {
	public:
		/// @brief 0～9のモデルとカメラを使い、指定ロケットの値を5桁で表示する。
		/// @param[in] digitModels 0から9までの数字モデル。
		/// @param[in] camera 画面固定配置に使うカメラ。
		/// @param[in] rocket 表示するエネルギーの取得元。
		EnergyView(
			const ScoreView::DigitModels& digitModels,
			const GameEngine::Camera* camera,
			const Rocket* rocket);
		~EnergyView() override = default;

		/// @brief 初期エネルギーを数字表示へ反映する。
		void Initialize() override;

		/// @brief ロケットの現在エネルギーとRegister設定を反映する。
		void Update() override;

		/// @brief 停止中も現在値とRegister設定を反映する。
		void DebugUpdate() override;

		/// @brief 数字モデルを描画キューへ登録する。
		void Draw() override;

	private:
		/// @brief ロケットの現在値をScoreViewへ渡し、Register設定を反映する。
		void SyncValue();

		const Rocket* rocket_ = nullptr; // 表示するEnergyの取得元
		ScoreView numberView_;           // 数字モデルの配置・色・5桁分解を担当する既存View
	};
}
