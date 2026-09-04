#pragma once

#include "IGameObject.h"

#include "Application/Score/ScoreView.h"

namespace GameEngine {
	class Camera;
}

namespace Prototype {
	class Rocket;

	/// <summary>
	/// 数字モデルを使ってロケットの現在エネルギーを画面へ表示する
	/// </summary>
	class EnergyView final : public GameEngine::IGameObject {
	public:
		EnergyView(
			const ScoreView::DigitModels& digitModels,
			const GameEngine::Camera* camera,
			const Rocket* rocket);
		~EnergyView() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

	private:
		void SyncValue();

		const Rocket* rocket_ = nullptr;
		ScoreView numberView_;
	};
}
