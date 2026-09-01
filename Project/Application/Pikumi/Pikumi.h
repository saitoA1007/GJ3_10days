#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Collider.h"

enum class PikumiState
{
    kFollow, // 追従
    kThrown, // 投げられて飛んでる
    kIdle    // 停止して回収待ち
};

class Pikumi : public GameEngine::IGameObject
{
public:
    Pikumi(GameEngine::Model* model);

    void Initialize() override;
    void Update() override;
    void Draw() override;

    // 投擲実行
    void Throw(const Vector3& direction, float initialSpeed);
    void RandomizeFormation();
    void Collect();

    // パラメータ変更用セッター
    void SetFollowSpeed(float speed) { followSpeed_ = speed; }
    void SetDampening(float damp) { dampening_ = damp; }
    void SetScale(float scale) { modelComponent_.worldTransform_.transform_.scale = { scale, scale, scale }; }
    void SetFieldRadius(float fieldRadius) { fieldRadius_ = fieldRadius; }

    // 状態と位置の設定
    void SetTargetFollowPosition(const Vector3& targetPos) { targetFollowPos_ = targetPos; }
    void SetPosition(const Vector3& pos) { modelComponent_.worldTransform_.transform_.translate = pos; }
    PikumiState GetState() const { return state_; }
    GameEngine::WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

    // 散らばり用のゲッター
    float GetAngleOffset() const { return angleOffset_; }
    float GetRadiusRatio() const { return radiusRatio_; }
    float GetSeed() const { return seed_; }
	Vector2 GetVelocity() const { return { velocity_.x, velocity_.z }; }

    GameEngine::SphereCollider& GetCollider() { return collider_; }

    // ハイライト設定
    void SetHighlight(bool enable);

private:
    // 衝突コールバック
    void OnCollisionEnter(const GameEngine::CollisionResult& result);

private:
    GameEngine::ModelComponent modelComponent_;
    GameEngine::SphereCollider collider_;
    PikumiState state_ = PikumiState::kFollow;

    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    Vector3 targetFollowPos_ = { 0.0f, 0.0f, 0.0f };

    float colliderRadius_ = 0.5f;
    float dampening_ = 0.95f; 
    float followSpeed_ = 10.0f; 
    float fieldRadius_ = 20.0f;
    float angleOffset_ = 0.0f;  
    float radiusRatio_ = 1.0f;  
    float seed_ = 0.0f;    
    float throwIgnorePlayerTimer_ = 0.0f;

    // Pikumi同士の当たり判定をするかどうか
    bool isPikumiCollisionEnabled_ = false;

    bool isHighlighted_ = false;
};