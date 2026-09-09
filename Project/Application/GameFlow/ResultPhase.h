#pragma once
#include "IGamePhase.h"
#include <Application/Result/ResultMessage.h>

class ResultPhase : public IGamePhase
{
public:
	
	ResultPhase() = default;

	void OnEnter(GameFlowContext& context) override;
	bool OnUpdate(GameFlowContext& context) override;

	const char* GetName() const override { return "Result"; }
	bool IsGameplayEnabled() const override { return false; }
	bool UsesGameSceneCamera() const override { return false; }

private:

	ResultMessage* resultMessage_ = nullptr;

};
