#pragma once
#include "IScene.h"
#include "Application/Scene/Transition/Fade.h"
#include "Camera.h"

class Player;

/// <summary>
/// プロトタイプ実装用のシーン
/// </summary>
class PrototypeScene : public GameEngine::IScene {
public:
	PrototypeScene();
	~PrototypeScene() override = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバッグ更新処理
	/// </summary>
	void DebugUpdate() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// シーンの終了状態を取得する
	/// </summary>
	bool IsFinished() override { return isFinished_; }

	/// <summary>
	/// 次に遷移するシーン名を取得する
	/// </summary>
	std::string NextSceneName() override { return "Game"; }

	/// <summary>
	/// シーン遷移演出を生成する
	/// </summary>
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:
	/// <summary>
	/// プレイヤー移動用の入力コマンドを登録する
	/// </summary>
	void InputRegisterCommand();

	bool isFinished_ = false;

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// プロトタイプ用プレイヤー
	Player* player_ = nullptr;
};
