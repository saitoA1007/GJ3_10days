#include "PrototypeEnergyView.h"

#include <cassert>

#include "Application/Prototype/Rocket/PrototypeRocket.h"

namespace Prototype {

	EnergyView::EnergyView(
		const ScoreView::DigitModels& digitModels,
		const GameEngine::Camera* camera,
		const Rocket* rocket)
		: rocket_(rocket), numberView_(digitModels, camera, "PrototypeEnergyView") {
		assert(rocket_ != nullptr && "Prototype energy view requires a rocket");
		SetUpdateOrder(40);
	}

	void EnergyView::Initialize() {
		SyncValue();
	}

	void EnergyView::Update() {
		SyncValue();
	}

	void EnergyView::DebugUpdate() {
		SyncValue();
	}

	void EnergyView::Draw() {
		numberView_.Draw(renderQueue_);
	}

	void EnergyView::SyncValue() {
		// 値の分解やモデル配置は既存ScoreViewへ任せ、このViewはRocketとの接続だけを担う。
		numberView_.SetValue(rocket_->GetEnergy());
		numberView_.Update();
	}
}
