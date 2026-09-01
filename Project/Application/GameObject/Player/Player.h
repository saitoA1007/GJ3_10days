#pragma once

#include "IGameObject.h"
#include "WorldTransform.h"

namespace GameEngine {
	class InputCommand;
	class Model;
}

/// <summary>
/// プレイヤー
/// </summary>
class Player : public GameEngine::IGameObject {
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model);
	~Player() override = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// プレイヤーのワールド変換を取得する
	/// </summary>
	GameEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }
	const GameEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

private:
	/// <summary>
	/// WASD入力による移動処理
	/// </summary>
	void Move();

	GameEngine::InputCommand* inputCommand_ = nullptr;
	GameEngine::Model* model_ = nullptr;
	GameEngine::WorldTransform worldTransform_;

	float moveSpeed_ = 5.0f;
};
