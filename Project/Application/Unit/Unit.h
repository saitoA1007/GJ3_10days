#pragma once

#include <cstdint>
#include <memory>

#include "ModelComponent.h"
#include "Collider.h"
#include "Vector3.h"
#include "Vector4.h"
#include "IGameObject.h"
#include "Application/Effect/RopeEffect.h"
#include "Application/Energy/EnergyPickup.h"

namespace GameEngine 
{
	class Model;
	class RenderQueue;
}

class Enemy;
class EnergyPickup;
class EnergySpawner;
class Rocket;

/// ユニット1体の行動状態
enum class UnitState : uint8_t
{
	Stored,            // ロケット内で待機中。再出撃可能
	MovingToEnergy,    // 予約したEnergyへ移動中
	MovingToEnemy,     // 予約したEnemyへ移動中
	MovingToPosition,
	ReturningToRocket, // Energyを頭上に載せて帰還中
	Blackhole,
};

/// 全ユニットで共有する移動・スタミナ・見た目の設定
struct UnitSettings
{
	Vector3 launchOffset = { 0.0f, 0.0f, 0.0f };        
	Vector3 scale = { 0.7f, 0.7f, 0.7f };               
	Vector3 carryOffset = { 0.0f, 1.45f, 0.0f };        
	float normalSpeed = 1.5f;                           
	float boostedSpeed = 5.0f;                          
	float pickupRadius = 0.45f;                         
	float deliveryRadius = 3.0f;
	float collisionRadius = 0.6f;                       
	float staminaDrainPerSecond = 2.0f;                 
	float distanceDrainRate = 0.08f;                    
	Vector4 normalColor = { 1.0f, 1.0f, 1.0f, 1.0f };   
	Vector4 staminaColor = { 0.25f, 0.85f, 1.0f, 1.0f };
	float groundY = 0.0f;                

	int32_t bhEnergyThreshold = 100;
	float bhDuration = 3.0f;
	float bhPullSpeed = 8.0f;
	float bhKillRadius = 0.5f;

	float bhBaseRadius = 10.0f;
	float bhNearDistance = 10.0f;
	float bhFarDistance = 30.0f;

	float bhRadiusNearMultiplier = 0.2f;
	float bhRadiusMidMultiplier = 1.0f;
	float bhRadiusFarMultiplier = 1.3f;

	float bhSpecialMultiplier = 5.0f;
};

/// @brief エネルギー回収または敵攻撃へ派遣されるユニット。
class Unit final : public GameEngine::IGameObject
{
public:
	// ==========================================
	// ライフサイクル・基本更新
	// ==========================================

	// 描画モデルと、エネルギー消費元となるロケットを受け取る。
	Unit(GameEngine::Model* model, GameEngine::Model* circleModel,
		Rocket* rocket, const UnitSettings* settings, GameEngine::Model* bameModel, uint32_t beamGH, EnergySpawner* energySpawner);

	// 待機状態と初期位置へ戻す
	void Initialize() override;

	// 現在状態に対応する移動処理を進める
	void Update() override;

	// 停止中も設定変更と運搬物位置を反映
	void RefreshVisual();

	// 出撃中のユニットを描画
	void Draw() override;


	// ==========================================
	// 出撃・行動制御
	// ==========================================

	// エネルギーを予約して回収へ出撃
	bool DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy);

	// 敵を予約して攻撃へ出撃する
	bool DispatchToEnemy(Enemy* target, int32_t requestedEnergy);

	// 指定した地面の目標座標へ派遣する
	bool DispatchToPosition(const Vector3& targetPosition, int32_t requestedEnergy);

	// 予約を解放して強制的に待機状態へ戻す
	void Recall();

	// 敵撃破時に生成されたEnergyを受け取り、そのまま帰還状態へ移行
	void StartCarryingEnergy(EnergyPickup* energy);


	// ==========================================
	// 撃破・被弾処理
	// ==========================================

	// 運搬物をその場へ落とし、ユニットを待機状態へ戻す
	// 運搬中のユニットを倒せた場合はtrue
	bool DefeatAndDropEnergy();

	// 倒れた個体を消費せず待機状態へ戻す
	void ReturnToStorageAfterDefeat();

	// ==========================================
	// ブラックホール
	// ==========================================

	// カーソルを合わせてエネルギーを注入
	bool InjectEnergy(int32_t requestedAmount);

	// ブラックホールの現在の有効半径を取得
	float GetBlackholeRadius() const;

	// 全オブジェクトのリストを渡して吸い込み処理を行う
	void ProcessBlackholeAbsorption(
		const std::vector<Enemy*>& enemies,
		const std::vector<Unit*>& units,
		const std::vector<EnergyPickup*>& energies,
		float deltaTime);

	bool IsBlackhole() const { return state_ == UnitState::Blackhole; }

	void Highlight();

	// ==========================================
	// 状態取得・判定
	// ==========================================

	// 再出撃できる待機状態か判定
	bool IsAvailable() const { return state_ == UnitState::Stored; }

	// フィールド上へ出撃中か判定
	bool IsDeployed() const {
		return state_ == UnitState::MovingToEnergy ||
			state_ == UnitState::MovingToEnemy ||
			state_ == UnitState::ReturningToRocket ||
			state_ == UnitState::MovingToPosition;
	}

	// エネルギーを持って帰還中か判定
	bool IsCarryingEnergy() const;

	UnitState     GetState()           const { return state_; }
	const Vector3& GetPosition()       const { return position_; }
	float         GetStamina()         const { return stamina_; }
	float         GetCollisionRadius() const { return settings_->collisionRadius; }
	EnergyPickup* GetTargetEnergy()    const { return targetEnergy_; }
	Enemy* GetTargetEnemy()     const { return targetEnemy_; }

private:
	// ==========================================
	// 内部処理 (状態別更新)
	// ==========================================
	void UpdateMovingToEnergy(float deltaTime);
	void UpdateMovingToEnemy(float deltaTime);
	void UpdateMovingToPosition(float deltaTime);
	void UpdateReturningToRocket(float deltaTime);
	void UpdateBlackhole(float deltaTime);
	void GenerateSpecialEnergy();

	// ==========================================
	// 内部処理 (移動・計算)
	// ==========================================
	void AllocateStamina(int32_t requestedEnergy);
	void MoveTowards(const Vector3& target, float deltaTime);
	void ConsumeStamina(float deltaTime);
	float DistanceSquaredXZ(const Vector3& a, const Vector3& b) const;
	void SyncModel();

private:
	// ==========================================
	// メンバ変数
	// ==========================================

	// 参照・設定
	Rocket* rocket_ = nullptr;                            
	const UnitSettings* settings_ = nullptr;               

	// コンポーネント
	std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
	GameEngine::SphereCollider collider_;                  

	// 状態・ターゲット
	UnitState state_ = UnitState::Stored;                
	EnergyPickup* targetEnergy_ = nullptr;                 
	Enemy* targetEnemy_ = nullptr;                        

	// パラメータ
	Vector3 position_ = {};                              
	float stamina_ = 0.0f;   
	float maxStamina_ = 0.0f;

	// 繋がっている演出
	RopeEffect RopeEffect_;

	// 目的地座標の保持用
	Vector3 targetPosition_ = {};

	// ブラックホール用変数
	float blackholeTimer_ = 0.0f; // ブラックホールの残り持続時間
	int32_t absorbedBasePoint_ = 0;  // 吸収した対象の重み付け合計

	EnergySpawner* energySpawner_ = nullptr;
	float highlightTimer_ = 0.0f;

	std::unique_ptr<GameEngine::ModelComponent> blackholeModel_;
	float bhEffectTimer_ = 0.0f;
};

