#pragma once
#include "IScene.h"

// エンジン機能をインクルード
#include "Camera.h"

#include "Application/Scene/Transition/Fade.h"
#include "Application/Score/Score.h"

class Player;
class ScoreView;
class EnergySpawner;
class EnemyManager;
class EnergyView;
class Field;
class GameFlow;
class LockOnController;
class Rocket;
class UnitManager;

namespace GameEngine
{
	class ControllerVibration;
	class DebugParameter;
}

class GameScene : public GameEngine::IScene {
public:
	GameScene();
	~GameScene();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="input"></param>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	void DebugUpdate() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 終了したことを伝える
	/// </summary>
	/// <returns></returns>
	bool IsFinished() override { return isFinished_; };

	/// <summary>
	/// 次のシーン遷移する場面の名前を取得
	/// </summary>
	/// <returns></returns>
	std::string NextSceneName() override { return "Title"; }	

	/// <summary>
	/// 遷移する演出
	/// </summary>
	/// <returns></returns>
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private: // シーン機能

	/// カメラ行列を更新してRenderQueueへ設定
	void UpdateCamera();

	std::unique_ptr<GameEngine::Camera> mainCamera_;                // 3D描画とマウスレイ投影に使うカメラ
	std::unique_ptr<GameEngine::DebugParameter> mainCameraDebugParameter_; // Translate / Rotate の確認・調整用
	Vector3 mainCameraEndRotation_ = {};                            // ロケット着地時のカメラ回転
	float mainCameraEntranceStartRotateX_ = 0.5f;                   // ロケット降下開始時のX回転
	// 終了フラグ
	bool isFinished_ = false;
	Player* player_ = nullptr;
	Field* field_ = nullptr;                             
	Rocket* rocket_ = nullptr;                           
	EnergySpawner* energySpawner_ = nullptr;   
	EnemyManager* enemyManager_ = nullptr;
	UnitManager* unitManager_ = nullptr;                 
	LockOnController* lockOnController_ = nullptr;       
	GameFlow* gameFlow_ = nullptr;   
	EnergyView* energyView_ = nullptr;
	Score score_;
	std::unique_ptr<ScoreView> scoreView_;
	std::unique_ptr<GameEngine::ControllerVibration> controllerVibration_;
	std::unique_ptr<GameEngine::Sprite> fadeSprite_;
	float fadeElapsedTime_ = 0.0f;

	// シーンライト
	float intensity_ = 1.0f;
	Vector3 dir_ = { 0.0f,-1.0f,0.5f };
	Vector4 lightColor_ = { 1.0f,1.0f,1.0f,1.0f };
private:

	/// <summary>
	/// 入力のコマンドを設定する
	/// </summary>
	void InputRegisterCommand();
};
