#pragma once
#include <IGameObject.h>
#include <ModelManager.h>
#include <AnimationManager.h>
#include <GameObjectManager.h>
#include "ShuffleNumber.h"
#include "ResultMessage.h"
#include <ModelComponent.h>

namespace GameEngine {
	class InputCommand;
}

class ResultStringManager : public GameEngine::IGameObject {
public:

	ResultStringManager(GameEngine::InputCommand* inputCommand, GameEngine::ModelManager* modelManager, GameEngine::AnimationManager* animationManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	void Boot(int score);

	bool CanControl() const { return currentType_ == Type::Control; }

	// シーンが終了することを伝える
	bool IsSceneFinished() const { 
		if (!isBoot_) { return false; }
		return isSceneFinished_;
	}

private:

	//
	// ShuffleNumber起動 <- ShuffleStopTime
	// 段々と停止させていく(maxDigitの個数) <- GetStopTime(digit)
	// メッセージ起動 <- GetMessageActivateTime()
	// 操作可能 <- GetControlTime() (もどるのモデルを表示する)
	//

	float GetStopTime(int digit) const { return shuffleStopTime_ + (digit * shuffleStopDelay_); }
	float GetMessageActivateTime() const { return messageActivateTime_ + GetStopTime(maxDigit_ - 1); }
	float GetControlTime() const { return messageActivateTime_ + GetMessageActivateTime(); }

	void RegistTransform(Transform& transform, std::string groop);

	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;
	bool isSceneFinished_ = false;

	const int aimScore_ = 100;
	const int aimScoreDistance_ = 1000000;

	std::vector<int> scoreDigits_;
	std::vector<std::unique_ptr<ShuffleNumber>> shuffleNumbers_;
	std::unique_ptr<ResultMessage> resultMessage_ = nullptr;

	std::unique_ptr<GameEngine::ModelComponent> kmModel_;
	std::unique_ptr<GameEngine::ModelComponent> spaceModel_;
	std::unique_ptr<GameEngine::ModelComponent> arrowModel_;

	const int maxDigit_ = 7;

	bool isBoot_ = false;
	float timer_ = 0.0f;

	GameEngine::DebugParameter debugParameter_{ "ResultStringManager" };

	float shuffleStopTime_ = 1.0f;
	float shuffleStopDelay_ = 0.3f;
	float messageActivateTime_ = 3.0f;
	float controlTime_ = 3.0f;

	Transform numberTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	float numberMergin_ = 0.5f;

	Transform kmTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform spaceTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform arrowTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	// シャッフル中の音
	uint32_t shuffleSH_ = 0;
	// 数字が決まった時の音
	uint32_t setNumSH_ = 0;
	// タイトル
	uint32_t backTitleSH_ = 0;

	enum Type {
		Shuffle,
		Stopping,
		Message,
		Control,
		Count
	} currentType_ = Type::Shuffle;

	ResultMessage::Type messageType_ = ResultMessage::Type::Mousukosi;
};
