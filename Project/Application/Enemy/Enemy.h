#pragma once
#include <IGameObject.h>
#include <WorldTransforms.h>
#include <Vector2.h>
#include <Collider.h>

class Rocket;
class Unit;
class UnitManager;
class EnergyPickup;
class EnergySpawner;
class Field;
enum class EnergySize : uint8_t;

enum class EnemyType {
	Straight_S,	//対象にまっすぐ近づく 小さい
	Straight_M,	//対象にまっすぐ近づく 中くらい
	Straight_L,	//対象にまっすぐ近づく 大きい

	Round,		//対象を中心に回る
	Snake,		//蛇行して対象に近づく

	Count
};

//EnemyManagerで更新を呼び出したいので、GameObjectManagerに登録しない。
class Enemy : public GameEngine::IGameObject {
public:
	struct Config {
		float speed_ = 2.0f;
		int hp = 1;
		float size_ = 1.0f;
		Vector4 normalColor_ = { 1.0f, 0.4f, 0.6f, 1.0f };
		Vector4 hitColor_ = { 0.9f, 0.0f, 0.0f, 1.0f };
		Vector4 highlightColor_ = { 1.0f, 0.9f, 0.4f, 1.0f }; // ハイライト用の色
	};

	/// 外部参照をまとめて保持する構造体
	struct Context {
		Field* field = nullptr;
		Rocket* rocket = nullptr;
		EnergySpawner* energySpawner = nullptr;
		UnitManager* unitManager = nullptr;
	};

public:
	Enemy(GameEngine::WorldTransforms::TransformData* data);
	~Enemy() = default;

	static void SetCollisionRadius(float radius) { collisionRadius_ = radius; }
	static void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }
	static void SetTrackingSpeedMultiplier(float multiplier) {
		trackingSpeedMultiplier_ = (multiplier < 0.0f) ? 0.0f : multiplier;
	}

	// 外部システムの参照を設定
	void SetContext(const Context& context) { context_ = context; }

	void SetUp(Vector2 position, Config config, EnemyType type, uint32_t effectID);

	void Initialize() override;
	void Update() override;
	void DeadUpdate();

	Vector3 GetPosition() const { return data_->transform.translate; }
	float GetCollisionRadius() const { return collisionRadius_ * config_.size_; }
	float GetDisplayScale() const { return config_.size_; }
	uint32_t GetEffectID() const { return effectID_; }
	Matrix4x4 GetWorldMatrix() const { return data_->worldMatrix; }

	void SetSnake(float width, float speed) { snakeWidth_ = width; snakeSpeed_ = speed; }
	void SetRound(float speed) { roundSpeed_ = speed; }
	void SetDamageTime(float time) { damageTime_ = time; }
	void SetMovementEnabled(bool enabled);
	void SetHighlighted(bool highlighted);

	bool IsTargetable() const { return isActive_ && !isDead_ && !isReservedForAttack_; }
	bool IsTargetCarrier() const { return isActive_ && !isDead_ && (targetUnit_ != nullptr); }

	// LockOn / ユニット派遣用連携機能
	bool WasDefeated() const { return wasDefeated_; }

	bool TryReserveForAttack();
	void CancelAttackReservation();

	// 撃破時エネルギー生成
	EnergyPickup* DefeatAndDropEnergy();
	// 運搬ユニットを追跡中かどうかを取得

	// ======== ブラックホール ========


	// ブラックホール等からの引き寄せ処理
	void PullTowards(const Vector3& targetPos, float speed, float deltaTime);
	// エネルギーをドロップせずに強制消滅
	void ForceDestroy() {
		Destroy();
	}
	// 吸い込み対象として有効かどうか判定
	bool IsAlive() const {
		return isActive_ && !isDead_;
	}

private:

	void DefaultMovement();
	void RoundMovement();
	void TrackingMovement(float deltaTime); // 運搬ユニット追跡移動

	void UpdateTarget();
	void RebaseMovementFromCurrentPosition();
	EnergySize GetDropEnergySize() const;

	void Destroy() override {
		isActive_ = false;
		isDead_ = true;
	}

	static inline float collisionRadius_ = 1.f;
	static inline float rotationSpeed_ = 1.5f;
	static inline float trackingSpeedMultiplier_ = 2.0f;

	GameEngine::WorldTransforms::TransformData* data_ = nullptr;
	GameEngine::SphereCollider collider_;

	Context context_; // 外部システムへの参照

	// 対象との距離・方向
	float distance_ = 0.0f;
	Vector2 direction_ = { 0.0f, 0.0f };

	float timer_ = 0.0f;

	int hp_ = 1;
	bool wasDefeated_ = false;
	bool isReservedForAttack_ = false; // ユニット攻撃の予約状態
	bool isHighlighted_ = false;        // ロックオンハイライト中か
	bool movementEnabled_ = true;       // falseなら現在座標で静止

	float damageTimer_ = 0.0f;
	float snakeTimer_ = 0.0f;
	float roundTimer_ = 0.0f;

	// 追跡用
	Unit* targetUnit_ = nullptr; // 追跡中の運搬ユニット
	float searchRadius_ = 8.0f;  // 運搬ユニットの索敵範囲

	//=== 設定項目 ====================================================
	float damageTime_ = 0.02f;
	Config config_;
	EnemyType type_;

	float snakeWidth_ = 0.0f;
	float snakeSpeed_ = 0.0f;
	float roundSpeed_ = 0.0f;

	// ブラックホール
	bool isBeingPulled_ = false;
	uint32_t effectID_ = 0; // 恒常処理のID
};
