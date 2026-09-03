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
class ImpactDetectionEffect;

class Player : public GameEngine::IGameObject 
{
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model, GameEngine::Model* pikumiModel, Field* field,
		ImpactDetectionEffect* impactDetectionEffect);
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
	void SetIsMoving(bool isMoving) { isMoving_ = isMoving; }
	float GetCurrentYaw() const { return currentYaw_; }
	int GetChargedPikumiCount() const { return chargedPikumiCount_; }
	int GetLastThrownCount() const { return lastThrownCount_; }
	uint32_t GetThrowEventId() const { return throwEventId_; }

public:

	// ワールド行列を取得
	GameEngine::WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

private:
	void ClampToField();
	void UpdateChargeThrow();
	void ClearAllPikumiHighlights();
	void UpdateMoveAnimation();

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
	float colliderRadius_;
	float colliderOffsetPosY_;
	float moveSpeed_;
	float currentYaw_;

	// Pikumi調整パラメータ
	int pikumiCount_ = 10;
	float pikumiScale_;        
	float pikumiFollowOffset_;
	float pikumiSpacing_ = 0.5f;      
	float pikumiSeparation_ = 0.4f;
	float pikumiFollowSpeed_;
	float pikumiThrowSpeed_;
	float pikumiDampening_;
	float pikumiCollectRadius_;
	float chargeTimer_;
	float maxChargeTime_;
	bool isCharging_ = false;
	int chargedPikumiCount_;
	int lastThrownCount_;
	uint32_t throwEventId_;

	// アニメーションのパラメータ
	Vector3 baseScale_ = { 3.0f,3.0f,3.0f };
	float squashFrequency_ = 14.0f;
	float squashStretchAmount_ = 0.25f;
	float pikumiJumpHeight_ = 0.4f;
	float pikumiJumpFrequency_ = 12.0f;
	float pikumiSquashAmount_ = 0.2f;

	// アニメーション変数
	float moveAnimTimer_ = 0.0f;
	bool isMoving_ = false;

private:

	/// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	void OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result);
};
