#pragma once

#include <cstdint>
#include <memory>

#include "Camera.h"
#include "IGameObject.h"
#include "Vector3.h"

namespace GameEngine
{
	class DebugParameter;
}

class Player;

/// <summary>
/// Playerを見下ろしながら追従するゲーム用カメラ。
/// </summary>
class GameCamera : public GameEngine::IGameObject
{
public:
	explicit GameCamera(Player* player);
	~GameCamera() override;

	void Initialize() override;
	void Update() override;

	GameEngine::Camera* GetCamera() { return &camera_; }
	const GameEngine::Camera* GetCamera() const { return &camera_; }
	void SetRotateWithMovement(bool enabled) { rotateWithMovement_ = enabled; }
	bool GetRotateWithMovement() const { return rotateWithMovement_; }
	void SetMoveToOriginWithHeightAnimation(bool enabled) { moveToOriginWithHeightAnimation_ = enabled; }
	bool GetMoveToOriginWithHeightAnimation() const { return moveToOriginWithHeightAnimation_; }
	void SetMoveTargetPosition(const Vector3& position) { moveTargetPosition_ = position; }
	const Vector3& GetMoveTargetPosition() const { return moveTargetPosition_; }

private:

	/// @brief 投げたときのHeight演出の状態
	enum class ThrowHeightAnimationState
	{
		Idle,      // 何もしていない状態
		Rising,    // 上昇中
		Holding,   // 高さを保持中
		Returning, // 元の高さに戻る途中
	};

	Vector3 CalculateCameraPosition(const Vector3& playerPosition) const;
	void LookAtPlayer(const Vector3& playerPosition);
	void StartThrowHeightAnimation();
	void UpdateThrowHeightAnimation(float deltaTime);

	Player* player_ = nullptr;
	GameEngine::Camera camera_;
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;

	float height_ = 28.0f;
	float distance_ = 18.0f;
	float lookAtHeight_ = 1.5f;
	float followSpeed_ = 8.0f;
	Vector3 rotateOffset_ = { 0.0f, 0.0f, 0.0f };
	bool rotateWithMovement_ = true;

	// Pikumiをまとめて投げたときのHeight演出
	int throwCountThreshold_ = 5;
	float riseHeight_ = 72.0f;
	float riseDuration_ = 1.0f;
	float holdDuration_ = 1.0f;
	float returnDuration_ = 1.0f;
	float returnHeight_ = 28.0f;
	bool moveToOriginWithHeightAnimation_ = true;
	Vector3 moveTargetPosition_ = { 0.0f, 0.0f, 0.0f };

	ThrowHeightAnimationState throwHeightAnimationState_ = ThrowHeightAnimationState::Idle;
	uint32_t lastHandledThrowEventId_ = 0;
	float throwHeightAnimationTimer_ = 0.0f;
	float throwHeightAnimationStart_ = 28.0f;
	float originMovementProgress_ = 0.0f;
	float originMovementStart_ = 0.0f;
};
