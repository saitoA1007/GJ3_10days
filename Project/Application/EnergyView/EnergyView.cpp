#include "EnergyView.h"

#include <cassert>

#include "Application/Rocket/Rocket.h"

EnergyView::EnergyView(
	const ScoreView::DigitModels& digitModels,
	const GameEngine::Camera* camera,
	const Rocket* rocket)
	: rocket_(rocket), numberView_(digitModels, camera, "EnergyView") 
{
	assert(rocket_ != nullptr && "energy view requires a rocket");
	SetUpdateOrder(40);
}

void EnergyView::Initialize() 
{
	SyncValue();
}

void EnergyView::Update() 
{
	SyncValue();
}

void EnergyView::DebugUpdate()
{
	SyncValue();
}

void EnergyView::Draw()
{
	numberView_.Draw(renderQueue_);
}

void EnergyView::SyncValue()
{
	// 値の分解やモデル配置は既存ScoreViewへ任せ、このViewはRocketとの接続だけを担う。
	numberView_.SetValue(rocket_->GetEnergy());
	numberView_.Update();
}

