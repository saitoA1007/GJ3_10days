#include "ResultStringManager.h"
#include <FPSCounter.h>
#include <EasingManager.h>
#include "AudioManager.h"
#include "InputCommand.h"
using namespace GameEngine;

namespace {
	//強制リニアの線形補完
	template<typename T>
	T lerp(const T& start, const T& end, float t) {
		return T(start + (end - start) * t);
	}
}

ResultStringManager::ResultStringManager(GameEngine::InputCommand* inputCommand, GameEngine::ModelManager* modelManager, GameEngine::AnimationManager* animationManager) {
	resultMessage_ = std::make_unique<ResultMessage>(modelManager, animationManager);

	inputCommand_ = inputCommand;

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

	RegistTransform(kmTransform_, "KM");
	RegistTransform(spaceTransform_, "Space");
	RegistTransform(arrowTransform_, "Arrow");

	kmModel_ = std::make_unique<GameEngine::ModelComponent>(modelManager->GetNameByModel("km.obj"));
	spaceModel_ = std::make_unique<GameEngine::ModelComponent>(modelManager->GetNameByModel("bottom0.obj"));
	arrowModel_ = std::make_unique<GameEngine::ModelComponent>(modelManager->GetNameByModel("ResultArrow.obj"));

	shuffleSH_ = AudioManager::GetInstance().GetHandleByName("resultShuffle.mp3");
	setNumSH_ = AudioManager::GetInstance().GetHandleByName("resultSetNum.mp3");
	backTitleSH_ = AudioManager::GetInstance().GetHandleByName("resultSetNum.mp3");
	bgmSH_ = AudioManager::GetInstance().GetHandleByName("clearBgm.mp3");
}

void ResultStringManager::Initialize() {
	resultMessage_->Initialize();
	isSceneFinished_ = false;
	for (auto& num : shuffleNumbers_) {
		num->Initialize();
	}

	debugParameter_.Apply();
}

void ResultStringManager::Update() {
	if (!isBoot_) {
		return;
	}

	debugParameter_.ApplyIfDirty();

	resultMessage_->DebugUpdate();
	for (int i = 0; i < maxDigit_; ++i) {
		Transform transform = numberTransform_;
		transform.translate.x += numberMergin_ * i;
		shuffleNumbers_[i]->SetTransform(transform);

		shuffleNumbers_[i]->DebugUpdate();
	}

	kmModel_->worldTransform_.transform_ = kmTransform_;
	kmModel_->Update();

	spaceModel_->worldTransform_.transform_ = spaceTransform_;
	spaceModel_->Update();

	arrowModel_->worldTransform_.transform_ = arrowTransform_;
	arrowModel_->Update();

	timer_ += GameEngine::FpsCounter::deltaTime;

	switch (currentType_) {
	case ResultStringManager::Shuffle:

		if (!AudioManager::GetInstance().IsPlay(shuffleSH_)) {
			AudioManager::GetInstance().Play(shuffleSH_, 0.2f, false);
		}

		if (timer_ >= shuffleStopTime_) {
			currentType_ = ResultStringManager::Stopping;
		}
		break;
	case ResultStringManager::Stopping:

		if (!AudioManager::GetInstance().IsPlay(shuffleSH_)) {
			AudioManager::GetInstance().Play(shuffleSH_, 0.2f, false);
		}

		for (int i = 0; i < maxDigit_; ++i) {
			if (timer_ >= GetStopTime(i)) {
				shuffleNumbers_[i]->Stop(scoreDigits_[i]);
			}
		}

		if (timer_ >= GetStopTime(maxDigit_ - 1)) {
			currentType_ = ResultStringManager::Message;
			resultMessage_->Boot(messageType_);
			AudioManager::GetInstance().Stop(shuffleSH_);
			AudioManager::GetInstance().Play(setNumSH_, 0.5f, false);
		}

		break;
	case ResultStringManager::Message:

		if (timer_ >= GetMessageActivateTime()) {
			currentType_ = ResultStringManager::Control;
		}

		break;

	case ResultStringManager::Control:
		//入力によってシーンの切り替えを行うことを許す。

		// 決定入力（スペースまたは左クリック）でタイトルへ
		if (inputCommand_->IsCommandActive("Decision")) {
			AudioManager::GetInstance().Play(backTitleSH_, 0.2f, false);
			isSceneFinished_ = true;

			// bgmを停止
			if (AudioManager::GetInstance().IsPlay(bgmSH_)) {
				AudioManager::GetInstance().Stop(bgmSH_);
			}
		}
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
	kmModel_->Draw(renderQueue_);

	if (currentType_ == Control) {
		spaceModel_->Draw(renderQueue_);
		arrowModel_->Draw(renderQueue_);
	}
}

void ResultStringManager::DebugUpdate() {
}

void ResultStringManager::Boot(int score) {
	isSceneFinished_ = false;
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

	if (score < aimScore_) {
		messageType_ = ResultMessage::Type::Mousukosi;
	} else {
		messageType_ = ResultMessage::Type::Tobisugi;
	}

	// bgmを再生
	AudioManager::GetInstance().Play(bgmSH_, 0.2f, true);
}

void ResultStringManager::RegistTransform(Transform& transform, std::string groop) {
	debugParameter_.Register("Scale", transform.scale, 0, groop);
	debugParameter_.Register("Rotate", transform.rotate, 1, groop);
	debugParameter_.Register("Translate", transform.translate, 2, groop);
}
