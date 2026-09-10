#include "EnergyView.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Application/Rocket/Rocket.h"
#include "Camera.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "RenderQueue.h"

EnergyView::EnergyView(
	const ScoreView::DigitModels& digitModels,
	const GameEngine::Camera* camera,
	const Rocket* rocket,
	GameEngine::Model* model)
	: rocket_(rocket),
	  camera_(camera),
	  numberView_(digitModels, camera, "EnergyView"),
	  debugParameter_("EnergyView"),
	  energyIcon_(model)
{
	assert(rocket_ != nullptr && "energy view requires a rocket");
	assert(camera_ != nullptr && "energy view requires a camera");
	debugParameter_.Register("ChangeDuration", changeDuration_, 5);
	debugParameter_.RegisterWorld("Icon", energyIcon_.worldTransform_);
	debugParameter_.Register("IconColor", energyIcon_.materialData_->color);
	debugParameter_.Apply();
	changeDuration_ = (std::max)(changeDuration_, 0.0f);
	SetUpdateOrder(40);

	energyIcon_.materialData_->enableLighting = false;
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

	// 数字と同じカメラ行列を掛け、カメラが動いても画面上の同じ位置へ表示する。
	const Matrix4x4 cameraWorld = renderQueue_->GetUseDebugCamera()
		? renderQueue_->GetDebugCameraWorldMatrix()
		: camera_->GetWorldMatrix();
	const Transform& iconTransform = energyIcon_.worldTransform_.transform_;
	energyIcon_.worldTransform_.UpdateWorldMatrix(
		GameEngine::Math::MakeAffineMatrix(
			iconTransform.scale, iconTransform.rotate, iconTransform.translate) * cameraWorld);
	energyIcon_.Draw(renderQueue_);
}

void EnergyView::SyncValue(float deltaTime)
{
	debugParameter_.ApplyIfDirty();
	changeDuration_ = (std::max)(changeDuration_, 0.0f);

	const int32_t latestValue = rocket_->GetEnergy();
	if (latestValue != targetValue_)
	{
		if (latestValue < targetValue_)
		{
			// ロックオン中の連続消費は実値へ即時追従させ、
			// チャージ上限へ到達した時点で表示にも全消費量を反映する。
			targetValue_ = latestValue;
			displayedValue_ = static_cast<float>(targetValue_);
			changeStartValue_ = displayedValue_;
			changeElapsedTime_ = changeDuration_;
		}
		else
		{
			// Energy獲得時は現在の表示値から最新値へ滑らかにつなぐ。
			changeStartValue_ = displayedValue_;
			targetValue_ = latestValue;
			changeElapsedTime_ = 0.0f;
		}
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
	// アイコンのワールド行列はカメラ追従込みでDrawが組み立てるため、ここでは更新しない。
}

