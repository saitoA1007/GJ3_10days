#pragma once

#include "IScene.h"

#include "Camera.h"

#include "Application/Scene/Transition/Fade.h"

namespace Prototype {
	class EnemyManager;
	class EnergySpawner;
	class EnergyView;
	class Field;
	class GameFlowController;
	class LockOnController;
	class Rocket;
	class UnitManager;
}

/// @brief 機能検証や仮組みに使用するプロトタイプ用シーン。
class ProtoScene final : public GameEngine::IScene {
public:
	/// @brief 入力、カメラ、Prototype用GameObjectを依存順に生成する。
	ProtoScene();
	~ProtoScene() override = default;

	/// @brief シーン状態とカメラ、背景を初期化する。
	void Initialize() override;

	/// @brief 通常更新中のカメラを同期する。
	void Update() override;

	/// @brief デバッグ停止中のカメラと原点ガイドを更新する。
	void DebugUpdate() override;

	/// @brief 描画は各GameObjectとStaticGameObjectManagerへ委譲する。
	void Draw() override;

	/// @brief シーン終了要求を取得する。
	/// @return 終了要求中ならtrue。
	bool IsFinished() override { return isFinished_; }

	/// @brief 次のシーン名を取得する。
	/// @return 現状は同じProtoシーン名。
	std::string NextSceneName() override { return "Proto"; }

	/// @brief シーン遷移時のフェード効果を生成する。
	/// @return 新しく生成したFade。
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:
	/// @brief マウス・キー・パッドを共通のプロト用コマンドへ登録する。
	void RegisterInputCommands();

	/// @brief カメラ行列を更新してRenderQueueへ設定する。
	void UpdateCamera();

	/// @brief デバッグ停止中も原点が分かるXYZ軸を描く。
	void DrawOriginGuide();

	bool isFinished_ = false;                                      // シーン遷移要求。現状は常にfalse
	std::unique_ptr<GameEngine::Camera> mainCamera_;                // 3D描画とマウスレイ投影に使うカメラ
	// 以下はGameObjectManagerが所有するため、シーン側では非所有ポインタとして参照する。
	Prototype::Field* field_ = nullptr;                             // 円形フィールドと領域判定
	Prototype::Rocket* rocket_ = nullptr;                           // 中心ロケットとEnergy保管庫
	Prototype::EnergySpawner* energySpawner_ = nullptr;             // Energyの落下生成とプール管理
	Prototype::UnitManager* unitManager_ = nullptr;                 // 回収・攻撃ユニットの管理
	Prototype::EnemyManager* enemyManager_ = nullptr;               // 外周から出現する敵の管理
	Prototype::LockOnController* lockOnController_ = nullptr;       // カーソル選択とユニット派遣
	Prototype::GameFlowController* gameFlowController_ = nullptr;   // 制限時間と一括停止制御
	Prototype::EnergyView* energyView_ = nullptr;                   // ロケットEnergyの画面表示
};
