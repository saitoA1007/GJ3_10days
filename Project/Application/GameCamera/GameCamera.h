#pragma once

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

private:
	Vector3 CalculateCameraPosition(const Vector3& playerPosition) const;
	void LookAtPlayer(const Vector3& playerPosition);

	Player* player_ = nullptr;
	GameEngine::Camera camera_;
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;

	float height_ = 28.0f;
	float distance_ = 18.0f;
	float lookAtHeight_ = 1.5f;
	float followSpeed_ = 8.0f;
	Vector3 rotateOffset_ = { 0.0f, 0.0f, 0.0f };
	bool rotateWithMovement_ = true;
};
