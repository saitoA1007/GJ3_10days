#pragma once
#include "IGameObject.h"
#include "Collider.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "InputCommand.h"

// 前方宣言
namespace GameEngine 
{
	class GameObjectManager;
}

class IPlayerState;

class Player : public GameEngine::IGameObject 
{
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model);
	~Player() = default;

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void ChangeState(std::unique_ptr<IPlayerState> newState);
	GameEngine::InputCommand* GetInputCommand() const { return inputCommand_; }
	float GetMoveSpeed() const { return moveSpeed_; }

public:

	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	std::unique_ptr<IPlayerState> currentState_;

	// 当たり判定
	float colliderRadius_ = 3.0f;
	// 当たり判定のオフセット
	float colliderOffsetPosY_ = 0.0f;

	float moveSpeed_ = 5.0f;

private:

	/// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	void OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result);
};