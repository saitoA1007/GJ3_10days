#include "UnitCountUI.h"

#include <algorithm>
#include <cassert>
#include <numbers>

#include "Application/Unit/UnitManager.h"

namespace
{
	/// @brief 数字と同じ向きから、Z軸まわりに傾けてスラッシュに見せる。
	ScoreView::Settings MakeSeparatorSettings()
	{
		ScoreView::Settings settings{};
		settings.digitCount = 1;
		settings.hideLeadingZeros = false;
		settings.registerLayoutParameters = false;
		settings.digitRotation = {
			std::numbers::pi_v<float> *0.5f,
			std::numbers::pi_v<float>,
			0.5f };
		return settings;
	}

	/// @brief ゼロ埋め表示にするため、先頭の0を省略しない設定を作る。
	ScoreView::Settings MakeNumberSettings()
	{
		ScoreView::Settings settings{};
		settings.hideLeadingZeros = false;
		settings.registerLayoutParameters = false;
		return settings;
	}
}

UnitCountUI::UnitCountUI(
	const ScoreView::DigitModels& digitModels,
	const GameEngine::Camera* camera,
	const UnitManager* unitManager)
	: unitManager_(unitManager),
	  availableView_(digitModels, camera, "UnitCountUI/Available", MakeNumberSettings()),
	  separatorView_(digitModels, camera, "UnitCountUI/Separator", MakeSeparatorSettings()),
	  totalView_(digitModels, camera, "UnitCountUI/Total", MakeNumberSettings())
{
	assert(unitManager_ != nullptr && "unit count ui requires a unit manager");
	debugParameter_.Register("Position", position_, 0);
	debugParameter_.Register("Scale", scale_, 1);
	debugParameter_.Register("DigitSpacing", digitSpacing_, 2);
	debugParameter_.Register("SeparatorGap", separatorGap_, 3);
	debugParameter_.Register("SeparatorScaleRate", separatorScaleRate_, 4);
	debugParameter_.Register("Color", color_, 5);
	debugParameter_.Apply();
	SanitizeSettings();

	// 区切り記号は1.objを傾けて流用するため、値は常に1で固定する。
	separatorView_.SetValue(1);
	SetUpdateOrder(40);
}

void UnitCountUI::Initialize()
{
	SyncValue();
}

void UnitCountUI::Update()
{
	SyncValue();
}

void UnitCountUI::DebugUpdate()
{
	SyncValue();
}

void UnitCountUI::Draw()
{
	availableView_.Draw(renderQueue_);
	separatorView_.Draw(renderQueue_);
	totalView_.Draw(renderQueue_);
}

void UnitCountUI::SyncValue()
{
	debugParameter_.ApplyIfDirty();
	SanitizeSettings();

	const int totalCount = static_cast<int>(unitManager_->GetUnitCount());
	const int availableCount = static_cast<int>(unitManager_->GetAvailableCount());

	// 総数の桁数に左右を揃えるので、5体なら1桁、12体なら2桁のゼロ埋め表示になる。
	const int digitCount = CountDigits(totalCount);
	availableView_.SetDigitCount(digitCount);
	totalView_.SetDigitCount(digitCount);
	availableView_.SetValue(availableCount);
	totalView_.SetValue(totalCount);

	ScoreView* views[] = { &availableView_, &separatorView_, &totalView_ };
	for (ScoreView* view : views)
	{
		view->SetScale(scale_);
		view->SetDigitSpacing(digitSpacing_);
		view->SetColor(color_);
	}
	separatorView_.SetScale(scale_ * separatorScaleRate_);

	// 桁数が変わっても間隔が崩れないよう、左端から順に配置し直す。
	float offsetX = 0.0f;
	availableView_.SetPosition(position_);
	offsetX += availableView_.GetWidth() + separatorGap_;
	separatorView_.SetPosition(position_ + Vector3{ offsetX, 0.0f, 0.0f });
	offsetX += separatorView_.GetWidth() + separatorGap_;
	totalView_.SetPosition(position_ + Vector3{ offsetX, 0.0f, 0.0f });

	for (ScoreView* view : views)
	{
		view->Update();
	}
}

void UnitCountUI::SanitizeSettings()
{
	scale_ = (std::max)(scale_, 0.0f);
	digitSpacing_ = (std::max)(digitSpacing_, 0.0f);
	separatorScaleRate_ = (std::max)(separatorScaleRate_, 0.0f);
}

int UnitCountUI::CountDigits(int value)
{
	int digitCount = 1;
	for (int remaining = (std::max)(value, 0) / 10; remaining > 0; remaining /= 10)
	{
		++digitCount;
	}
	return (std::min)(digitCount, ScoreView::kMaxDigitCount);
}
