#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Application/GameFlow/TutorialPhase.h"
#include "TutorialCameraModelView.h"

class GameFlow;
class LockOnController;

namespace GameEngine
{
	class Camera;
	class InputCommand;
	class Model;
	class RenderQueue;
}

/// @brief TutorialPhaseの工程に合わせ、チュートリアルTextを順番に表示する。
class TutorialTextSequence final
{
public:
	enum class ProgressMode
	{
		PhaseStep,
		TimedHold,
	};

	struct StepDefinition
	{
		ProgressMode progressMode = ProgressMode::PhaseStep;
		TutorialPhase::Step phaseStep = TutorialPhase::Step::SelectEnergy;
		GameEngine::Model* model = nullptr;
		std::string parameterGroupName;
		TutorialCameraModelView::Settings viewSettings;
		float successTargetRotationX = 8.021f;
		float successRotateDuration = 0.5f;
		float successReturnDuration = 0.5f;
		float errorDuration = 0.35f;
		float shakeAmplitude = 0.45f;
		float shakeCycles = 4.0f;
		bool showSuccessColor = true;
	};

	TutorialTextSequence(
		const GameEngine::Camera* camera,
		const GameFlow* gameFlow,
		const LockOnController* lockOnController,
		const GameEngine::InputCommand* inputCommand,
		std::vector<StepDefinition> definitions);
	~TutorialTextSequence();

	void Reset();
	void Update(bool advanceAnimation, float deltaTime);
	void Draw(GameEngine::RenderQueue* renderQueue);

private:
	struct StepEntry;

	const TutorialPhase* GetTutorialPhase() const;
	bool HasPhaseStepCompleted(const StepEntry& entry, const TutorialPhase& phase) const;
	bool IsMistakeInput(const StepEntry& entry) const;
	void ResetSteps();

	const GameFlow* gameFlow_ = nullptr;
	const LockOnController* lockOnController_ = nullptr;
	const GameEngine::InputCommand* inputCommand_ = nullptr;
	std::vector<std::unique_ptr<StepEntry>> steps_;
	size_t currentStepIndex_ = 0;
	bool isActive_ = false;
	bool wasInTutorial_ = false;
};
