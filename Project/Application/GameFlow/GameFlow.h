#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"
#include "IGamePhase.h"

class GameFlow final : public GameEngine::IGameObject
{
public:
	GameFlow(const GameFlowContext& context, const GameFlowSettings& settings = {});
	~GameFlow() override = default;

	void Initialize() override;
	void Update() override;
	void DebugUpdate() override;
	void Draw() override {}

	void AdvanceToNextPhase();
	void ChangePhase(size_t index);

	IGamePhase* GetCurrentPhase() const
	{
		return (currentPhaseIndex_ < phases_.size()) ? phases_[currentPhaseIndex_].get() : nullptr;
	}

	const GameFlowContext& GetContext() const { return context_; }
	bool UsesGameSceneCamera() const
	{
		const IGamePhase* current = GetCurrentPhase();
		return current ? current->UsesGameSceneCamera() : true;
	}

private:
	void ApplyGameplayState();
	void ApplyDebugParameters();
	void BuildPhases(); // フェーズ配列の組み立て

	GameFlowContext context_;
	GameFlowSettings settings_;
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;

	std::vector<std::unique_ptr<IGamePhase>> phases_;
	size_t currentPhaseIndex_ = 0;
	bool debugPaused_ = false;
};
