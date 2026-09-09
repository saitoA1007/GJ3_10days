#include "EnergyView.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Application/Rocket/Rocket.h"
#include "FPSCounter.h"

EnergyView::EnergyView(
	const ScoreView::DigitModels& digitModels,
	const GameEngine::Camera* camera,
	const Rocket* rocket)
	: rocket_(rocket),
	  numberView_(digitModels, camera, "EnergyView"),
	  debugParameter_("EnergyView")
{
	assert(rocket_ != nullptr && "energy view requires a rocket");
	debugParameter_.Register("ChangeDuration", changeDuration_, 5);
	debugParameter_.Apply();
	changeDuration_ = (std::max)(changeDuration_, 0.0f);
	SetUpdateOrder(40);
}

void EnergyView::Initialize() 
{
	// シーン開始時は初期値を即時表示し、初期化自体をカウント変動に見せない。
	targetValue_ = rocket_->GetEnergy();
	displayedValue_ = static_cast<float>(targetValue_);
	changeStartValue_ = displayedValue_;
	changeElapsedTime_ = changeDuration_;
	SyncValue(0.0f);
}

void EnergyView::Update() 
{
	SyncValue(GameEngine::FpsCounter::gameDeltaTime);
}

void EnergyView::DebugUpdate()
{
	// ゲーム停止中は新しい目標値だけを受け取り、補間時間は進めない。
	SyncValue(0.0f);
}

void EnergyView::Draw()
{
	numberView_.Draw(renderQueue_);
}

void EnergyView::SyncValue(float deltaTime)
{
	debugParameter_.ApplyIfDirty();
	changeDuration_ = (std::max)(changeDuration_, 0.0f);

	const int32_t latestValue = rocket_->GetEnergy();
	if (latestValue != targetValue_)
	{
		// 連続して増減しても表示値を飛ばさず、現在位置から最新値へつなぎ直す。
		changeStartValue_ = displayedValue_;
		targetValue_ = latestValue;
		changeElapsedTime_ = 0.0f;
	}

	if (changeDuration_ <= 0.0f)
	{
		displayedValue_ = static_cast<float>(targetValue_);
	}
	else if (displayedValue_ != static_cast<float>(targetValue_))
	{
		changeElapsedTime_ = (std::min)(
			changeElapsedTime_ + (std::max)(deltaTime, 0.0f),
			changeDuration_);
		const float progress = changeElapsedTime_ / changeDuration_;
		displayedValue_ = changeStartValue_ +
			(static_cast<float>(targetValue_) - changeStartValue_) * progress;

		if (changeElapsedTime_ >= changeDuration_)
		{
			displayedValue_ = static_cast<float>(targetValue_);
		}
	}

	// 値の分解やモデル配置は既存ScoreViewへ任せる。
	numberView_.SetValue(static_cast<int>(std::round(displayedValue_)));
	numberView_.Update();
}

