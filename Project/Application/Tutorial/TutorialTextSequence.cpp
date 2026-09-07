#include "TutorialTextSequence.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include "InputCommand.h"

#include "Application/GameFlow/GameFlow.h"
#include "Application/LockOn/LockOnController.h"

namespace
{
	constexpr float kTwoPi = 6.28318531f;
	constexpr const char* kLockOnTriggerCommand = "LockOnTrigger";
	constexpr const char* kLockOnReleaseCommand = "LockOnRelease";
	constexpr Vector3 kNormalColor = { 1.0f, 1.0f, 1.0f };
	constexpr Vector3 kSuccessColor = { 0.0f, 1.0f, 0.0f };
	constexpr Vector3 kErrorColor = { 1.0f, 0.0f, 0.0f };
}

struct TutorialTextSequence::StepEntry
{
	std::unique_ptr<TutorialCameraModelView> view;
	ProgressMode progressMode = ProgressMode::PhaseStep;
	TutorialPhase::Step phaseStep = TutorialPhase::Step::SelectEnergy;
	float successTargetRotationX = 8.021f;
	float successRotateDuration = 0.5f;
	float successReturnDuration = 0.5f;
	float errorDuration = 0.35f;
	float shakeAmplitude = 0.45f;
	float shakeCycles = 4.0f;
	float errorElapsed = 0.35f;
	float holdElapsed = 0.0f;
	bool successStarted = false;
	bool showSuccessColor = true;
	bool activated = false;
	std::function<void()> onActivated;
};

TutorialTextSequence::TutorialTextSequence(
	const GameEngine::Camera* camera,
	const GameFlow* gameFlow,
	const LockOnController* lockOnController,
	const GameEngine::InputCommand* inputCommand,
	std::vector<StepDefinition> definitions)
	: gameFlow_(gameFlow),
	lockOnController_(lockOnController),
	inputCommand_(inputCommand)
{
	assert(camera && "tutorial text sequence requires a camera.");
	assert(gameFlow_ && "tutorial text sequence requires GameFlow.");
	assert(lockOnController_ && "tutorial text sequence requires LockOnController.");
	assert(inputCommand_ && "tutorial text sequence requires InputCommand.");

	steps_.reserve(definitions.size());
	for (StepDefinition& definition : definitions)
	{
		auto entry = std::make_unique<StepEntry>();
		entry->view = std::make_unique<TutorialCameraModelView>(
			definition.model,
			camera,
			definition.parameterGroupName,
			definition.viewSettings);
		entry->progressMode = definition.progressMode;
		entry->phaseStep = definition.phaseStep;
		entry->successTargetRotationX = definition.successTargetRotationX;
		entry->successRotateDuration = (std::max)(definition.successRotateDuration, 0.0f);
		entry->successReturnDuration = (std::max)(definition.successReturnDuration, 0.0f);
		entry->errorDuration = (std::max)(definition.errorDuration, 0.0f);
		entry->shakeAmplitude = (std::max)(definition.shakeAmplitude, 0.0f);
		entry->shakeCycles = (std::max)(definition.shakeCycles, 0.0f);
		entry->errorElapsed = entry->errorDuration;
		entry->showSuccessColor = definition.showSuccessColor;
		entry->onActivated = std::move(definition.onActivated);
		steps_.push_back(std::move(entry));
	}
	Reset();
}

TutorialTextSequence::~TutorialTextSequence() = default;

void TutorialTextSequence::Reset()
{
	ResetSteps();
	isActive_ = false;
	wasInTutorial_ = false;
}

void TutorialTextSequence::Update(bool advanceAnimation, float deltaTime)
{
	const TutorialPhase* tutorialPhase = GetTutorialPhase();
	const bool isTutorial = tutorialPhase != nullptr;
	if (isTutorial && !wasInTutorial_)
	{
		ResetSteps();
	}

	isActive_ = isTutorial;
	wasInTutorial_ = isTutorial;
	if (!isActive_ || currentStepIndex_ >= steps_.size())
	{
		return;
	}

	StepEntry& entry = *steps_[currentStepIndex_];
	if (!entry.activated)
	{
		entry.activated = true;
		if (entry.onActivated)
		{
			entry.onActivated();
		}
	}

	if (!entry.successStarted && entry.progressMode == ProgressMode::PhaseStep &&
		HasPhaseStepCompleted(entry, *tutorialPhase))
	{
		entry.successStarted = true;
		entry.errorElapsed = entry.errorDuration;
		entry.view->StartSuccessAnimation(
			entry.successTargetRotationX,
			entry.successRotateDuration,
			entry.successReturnDuration);
	}
	else if (advanceAnimation && !entry.successStarted && IsMistakeInput(entry))
	{
		entry.errorElapsed = 0.0f;
	}

	if (advanceAnimation && entry.errorElapsed < entry.errorDuration)
	{
		entry.errorElapsed = (std::min)(
			entry.errorElapsed + (std::max)(deltaTime, 0.0f),
			entry.errorDuration);
	}

	const bool isErrorFeedbackActive = entry.errorElapsed < entry.errorDuration;
	entry.view->SetColor(
		isErrorFeedbackActive
		? kErrorColor
		: (entry.successStarted && entry.showSuccessColor ? kSuccessColor : kNormalColor));

	float shakeOffsetX = 0.0f;
	if (isErrorFeedbackActive && entry.errorDuration > 0.0f)
	{
		const float progress = entry.errorElapsed / entry.errorDuration;
		shakeOffsetX = std::sin(progress * kTwoPi * entry.shakeCycles) *
			entry.shakeAmplitude * (1.0f - progress);
	}
	entry.view->SetDisplayOffset({ shakeOffsetX, 0.0f, 0.0f });
	entry.view->Update(true, advanceAnimation, deltaTime);

	if (!entry.successStarted && entry.progressMode == ProgressMode::TimedHold &&
		entry.view->IsEntranceAnimationComplete())
	{
		if (advanceAnimation)
		{
			entry.holdElapsed += (std::max)(deltaTime, 0.0f);
		}
		if (entry.holdElapsed >= entry.view->GetHoldDuration())
		{
			entry.successStarted = true;
			entry.view->StartReturnAnimation(entry.view->GetMoveDuration());
		}
	}

	if (entry.successStarted && entry.view->IsSuccessAnimationComplete())
	{
		++currentStepIndex_;
	}
}

void TutorialTextSequence::Draw(GameEngine::RenderQueue* renderQueue)
{
	if (!isActive_ || currentStepIndex_ >= steps_.size())
	{
		return;
	}
	steps_[currentStepIndex_]->view->Draw(renderQueue);
}

const TutorialPhase* TutorialTextSequence::GetTutorialPhase() const
{
	if (!gameFlow_)
	{
		return nullptr;
	}
	return dynamic_cast<const TutorialPhase*>(gameFlow_->GetCurrentPhase());
}

bool TutorialTextSequence::HasPhaseStepCompleted(
	const StepEntry& entry,
	const TutorialPhase& phase) const
{
	return static_cast<int>(phase.GetStep()) > static_cast<int>(entry.phaseStep);
}

bool TutorialTextSequence::IsMistakeInput(const StepEntry& entry) const
{
	if (entry.progressMode != ProgressMode::PhaseStep)
	{
		return false;
	}
	if (!inputCommand_)
	{
		return false;
	}

	switch (entry.phaseStep)
	{
	case TutorialPhase::Step::SelectEnergy:
		return inputCommand_->IsCommandActive(kLockOnTriggerCommand) &&
			(!lockOnController_ || !lockOnController_->GetSelectedEnergy());
	case TutorialPhase::Step::DispatchUnit:
	case TutorialPhase::Step::WaitForChargeInstruction:
	case TutorialPhase::Step::WaitForEnemyCollisionInstruction:
	case TutorialPhase::Step::EnemyCollision:
		return false;
	case TutorialPhase::Step::ChargeEnergy:
		return inputCommand_->IsCommandActive(kLockOnReleaseCommand);
	case TutorialPhase::Step::Complete:
		return false;
	}
	return false;
}

void TutorialTextSequence::ResetSteps()
{
	currentStepIndex_ = 0;
	for (const auto& entry : steps_)
	{
		entry->view->Reset();
		entry->view->SetColor(kNormalColor);
		entry->errorElapsed = entry->errorDuration;
		entry->holdElapsed = 0.0f;
		entry->successStarted = false;
		entry->activated = false;
	}
}
