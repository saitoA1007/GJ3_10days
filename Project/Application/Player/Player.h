#pragma once
#include <cstdint>
#include <vector>
#include "IGameObject.h"
#include "Collider.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "InputCommand.h"
#include "Application/Pikumi/Pikumi.h"

// 前方宣言
namespace GameEngine 
{
	class GameObjectManager;
}

class IPlayerState;
class Field;

class Player : public GameEngine::IGameObject 
{
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model, GameEngine::Model* pikumiModel, Field* field);
	~Player() = default;

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void UpdatePikumiFormations();
	void ThrowAllPikumis();
	void CheckPikumiCollection();

	void ChangeState(std::unique_ptr<IPlayerState> newState);
	GameEngine::InputCommand* GetInputCommand() const { return inputCommand_; }
	float GetMoveSpeed() const { return moveSpeed_; }

	void SetCurrentYaw(float yaw) { currentYaw_ = yaw; }
	float GetCurrentYaw() const { return currentYaw_; }
	int GetLastThrownCount() const { return lastThrownCount_; }
	uint32_t GetThrowEventId() const { return throwEventId_; }

public:

	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

private:
	void ClampToField();
	void UpdateChargeThrow();
	void ClearAllPikumiHighlights();

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// 現在のState
	std::unique_ptr<IPlayerState> currentState_;

	// Pikumi兵
	GameEngine::Model* pikumiModel_ = nullptr;
	std::vector<std::unique_ptr<Pikumi>> pikumis_;

	// フィールドのポインタ
	Field* field_ = nullptr;

	// プレイヤー調整パラメータ
	float colliderRadius_ = 3.0f;
	float colliderOffsetPosY_ = 0.0f;
	float moveSpeed_ = 5.0f;
	float currentYaw_ = 0.0f;

	// Pikumi調整パラメータ
	int pikumiCount_ = 10;
	float pikumiScale_ = 1.0f;        
	float pikumiFollowOffset_ = 2.5f;
	float pikumiRadius_ = 2.5f;
	float pikumiFollowSpeed_ = 10.0f;
	float pikumiThrowSpeed_ = 25.0f;
	float pikumiDampening_ = 0.92f;
	float pikumiCollectRadius_ = 3.0f;
	float chargeTimer_ = 0.0f;
	float maxChargeTime_ = 1.5f;
	bool isCharging_ = false;
	int lastThrownCount_ = 0;
	uint32_t throwEventId_ = 0;

private:

	/// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	void OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result);
};
