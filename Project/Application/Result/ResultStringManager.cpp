#include "ResultStringManager.h"
#include <FPSCounter.h>
#include <EasingManager.h>

namespace {
	//強制リニアの線形補完
	template<typename T>
	T lerp(const T& start, const T& end, float t) {
		return T(start + (end - start) * t);
	}
}

ResultStringManager::ResultStringManager(GameEngine::ModelManager* modelManager, GameEngine::AnimationManager* animationManager) {
	resultMessage_ = std::make_unique<ResultMessage>(modelManager, animationManager);

	for (uint32_t i = 0; i < 7; ++i) {
		auto& num = shuffleNumbers_.emplace_back(std::make_unique<ShuffleNumber>(modelManager, i));
		num->SetActive(false);
	}

	debugParameter_.Register("ShuffleStopTime", shuffleStopTime_, 0, "Time");
	debugParameter_.Register("ShuffleStopDelay", shuffleStopDelay_, 1, "Time");
	debugParameter_.Register("MessageActivateTime", messageActivateTime_, 2, "Time");
	debugParameter_.Register("ControlTime", controlTime_, 3, "Time");

	debugParameter_.Register("Scale", numberTransform_.scale, 0, "Number");
	debugParameter_.Register("Rotate", numberTransform_.rotate, 1, "Number");
	debugParameter_.Register("Translate", numberTransform_.translate, 2, "Number");
	debugParameter_.Register("Mergin", numberMergin_, 3, "Number");

	debugParameter_.Register("Scale", kmTransform_.scale, 0, "KM");
	debugParameter_.Register("Rotate", kmTransform_.rotate, 1, "KM");
	debugParameter_.Register("Translate", kmTransform_.translate, 2, "KM");

	kmModel_ = std::make_unique<GameEngine::ModelComponent>(modelManager->GetNameByModel("km.obj"));
}

void ResultStringManager::Initialize() {
	resultMessage_->Initialize();
	for (auto& num : shuffleNumbers_) {
		num->Initialize();
	}

	debugParameter_.Apply();
}

void ResultStringManager::Update() {
	if (!isBoot_) {
		return;
	}

	timer_ += GameEngine::FpsCounter::deltaTime;

	switch (currentType_) {
	case ResultStringManager::Shuffle:
		if (timer_ >= shuffleStopTime_) {
			currentType_ = ResultStringManager::Stopping;
		}
		break;
	case ResultStringManager::Stopping:

		for (int i = 0; i < maxDigit_; ++i) {
			if (timer_ >= GetStopTime(i)) {
				shuffleNumbers_[i]->Stop(scoreDigits_[i]);
			}
		}

		if (timer_ >= GetStopTime(maxDigit_ - 1)) {
			currentType_ = ResultStringManager::Message;
			resultMessage_->Boot(ResultMessage::Type::Mousukosi);
		}

		break;
	case ResultStringManager::Message:

		if (timer_ >= GetMessageActivateTime()) {
			currentType_ = ResultStringManager::Control;
		}

		break;
	case ResultStringManager::Control:

		//入力によってシーンの切り替えを行うことを許す。

		break;
	}

	resultMessage_->Update();
	for (auto& num : shuffleNumbers_) {
		num->Update();
	}
}

void ResultStringManager::Draw() {
	if (!isBoot_) {
		return;
	}
	resultMessage_->Draw();
	for (auto& num : shuffleNumbers_) {
		num->Draw();
	}
}

void ResultStringManager::DebugUpdate() {
	debugParameter_.ApplyIfDirty();

	resultMessage_->DebugUpdate();
	for (int i = 0; i < maxDigit_; ++i) {
		Transform transform = numberTransform_;
		transform.translate.x += numberMergin_ * i;
		shuffleNumbers_[i]->SetTransform(transform);

		shuffleNumbers_[i]->DebugUpdate();
	}

	kmModel_->worldTransform_.transform_ = kmTransform_;
}

void ResultStringManager::Boot(int score) {
	isBoot_ = true;
	for (auto& num : shuffleNumbers_) {
		num->ShuffleStart();
	}

	int direction = 0;
	float t = (float)score / (float)aimScore_;
	int lerped = lerp(0, aimScoreDistance_, t);

	scoreDigits_.clear();
	scoreDigits_.reserve(maxDigit_);
	for (int i = 0; i < maxDigit_; ++i) {
		int digit = lerped % 10;
		if (lerped == 0 && i > 0) {
			digit = -1; // 表示しない
		}
		scoreDigits_.push_back(digit);
		lerped /= 10;
	}
}
