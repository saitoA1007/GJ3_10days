#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "Application/Utils/GameTimer.h"
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
	// 全個数の設定を一括変更する。取得時は現在選択されている個数の設定を返す。
	void SetMoveToOriginWithHeightAnimation(bool enabled)
	{
		for (auto& settings : throwHeightAnimationSettings_)
		{
			settings.moveToOrigin = enabled;
		}
	}
	bool GetMoveToOriginWithHeightAnimation() const
	{
		return throwHeightAnimationSettings_[activeThrowHeightAnimationIndex_].moveToOrigin;
	}
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

	struct ThrowHeightAnimationSettings
	{
		float riseHeight = 80.0f;
		float riseDuration = 0.2f;
		float holdDuration = 2.0f;
		float returnDuration = 0.5f;
		float returnHeight = 36.0f;
		bool moveToOrigin = true;
	};

	static constexpr int kMinThrowAnimationCount = 2;
	static constexpr int kMaxThrowAnimationCount = 5;

	Vector3 CalculateCameraPosition(const Vector3& playerPosition) const;
	void LookAtPlayer(const Vector3& playerPosition);
	void StartThrowHeightAnimation(int thrownCount);
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

	// 2・3・4・5個以上の投擲ごとにHeight演出を調整する。
	std::array<ThrowHeightAnimationSettings, kMaxThrowAnimationCount - kMinThrowAnimationCount + 1>
		throwHeightAnimationSettings_ = {{ { 48.0f }, { 60.0f }, { 70.0f }, { 80.0f } }};
	int activeThrowHeightAnimationIndex_ = 0;
	Vector3 moveTargetPosition_ = { 0.0f, 0.0f, 0.0f };

	ThrowHeightAnimationState throwHeightAnimationState_ = ThrowHeightAnimationState::Idle;
	uint32_t lastHandledThrowEventId_ = 0;
	GameTimer throwHeightAnimationTimer_;
	float throwHeightAnimationStart_ = 28.0f;
	float originMovementProgress_ = 0.0f;
	float originMovementStart_ = 0.0f;
};
