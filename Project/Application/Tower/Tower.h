#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Collider.h"
#include "DebugParameter.h"

class Tower : public GameEngine::IGameObject
{
public:
    Tower(GameEngine::Model* model);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugUpdate() override;

    // ダメージ処理
    void TakeDamage(float damage);

    // ゲッター
    float GetHp() const { return hp_; }
    float GetMaxHp() const { return maxHp_; }
    bool IsDead() const { return isDead_; }
    GameEngine::SphereCollider& GetCollider() { return collider_; }

private:
    void OnCollisionEnter(const GameEngine::CollisionResult& result);

private:
    GameEngine::ModelComponent modelComponent_;
    GameEngine::SphereCollider collider_;

    // パラメータ機能
    std::unique_ptr<GameEngine::DebugParameter> debugParame_;

    // 調整パラメータ
    float maxHp_ = 100.0f;
    float hp_ = 100.0f;
    float colliderRadius_ = 2.0f;
    bool isDead_ = false;
};