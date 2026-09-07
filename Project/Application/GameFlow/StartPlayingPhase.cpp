#include "StartPlayingPhase.h"

#include <algorithm>

#include "FPSCounter.h"
#include "Application/StartPlaying/StartPlayingView.h"
#include "Application/Tutorial/TutorialCameraModelView.h"

void StartPlayingPhase::OnEnter(GameFlowContext& context)
{
	remainingTime_ = context.settings
		? (std::max)(context.settings->startPlayingDuration, 0.0f)
		: 0.0f;

	if (context.tutorialLogoView)
	{
		context.tutorialLogoView->StartReturnAnimation(
			context.tutorialLogoView->GetMoveDuration());
	}
	if (context.startPlayingView)
	{
		context.startPlayingView->Start();
	}
}

bool StartPlayingPhase::OnUpdate(GameFlowContext& context)
{
	const float deltaTime = (std::max)(GameEngine::FpsCounter::deltaTime, 0.0f);
	remainingTime_ = (std::max)(remainingTime_ - deltaTime, 0.0f);
	if (context.startPlayingView)
	{
		context.startPlayingView->Update(deltaTime);
	}
	return remainingTime_ <= 0.0f;
}

void StartPlayingPhase::OnExit(GameFlowContext& context)
{
	if (context.startPlayingView)
	{
		context.startPlayingView->Stop();
	}
}
