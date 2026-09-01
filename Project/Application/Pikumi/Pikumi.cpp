#include "Pikumi.h"

#include <random>

#include "FPSCounter.h"
#include "EasingManager.h"
#include "Application/CollisionConfig.h"

using namespace GameEngine;

Pikumi::Pikumi(GameEngine::Model* model) : modelComponent_(model)
{
    modelComponent_.worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> radiusDist(0.2f, 1.0f);    
    std::uniform_real_distribution<float> seedDist(0.0f, 100.0f);   

    angleOffset_ = angleDist(gen);
    radiusRatio_ = radiusDist(gen);
    seed_ = seedDist(gen);

    // コライダーの初期化
    collider_.SetRadius(colliderRadius_);
    collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
    collider_.SetCollisionAttribute(kCollisionAttributePikumi); 
    collider_.SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributePlayer);

    UserData userData;
    userData.typeID = static_cast<uint32_t>(CollisionTypeID::kPikumi);
    userData.object = this;
    collider_.SetUserData(userData);

    collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
        this->OnCollisionStay(result);
        });
}

void Pikumi::Initialize()
{
    state_ = PikumiState::kFollow;
    velocity_ = { 0.0f, 0.0f, 0.0f };
}

void Pikumi::Throw(const Vector3& direction, float initialSpeed)
{
    state_ = PikumiState::kThrown;

    Vector3 dir = direction; 
    dir.Normalize();      

    velocity_ = dir * initialSpeed;

    // 投げた直後0.05秒間はプレイヤーとの衝突処理を無視
    throwIgnorePlayerTimer_ = 0.05f;
}

void Pikumi::RandomizeFormation()
{
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> radiusDist(0.2f, 1.0f);
    std::uniform_real_distribution<float> seedDist(0.0f, 100.0f);

    angleOffset_ = angleDist(gen);
    radiusRatio_ = radiusDist(gen);
    seed_ = seedDist(gen);
}

void Pikumi::Collect()
{
    state_ = PikumiState::kFollow;
    RandomizeFormation(); 
}

void Pikumi::Update()
{
    if (throwIgnorePlayerTimer_ > 0.0f)
    {
        throwIgnorePlayerTimer_ -= FpsCounter::deltaTime;
    }

    switch (state_)
    {
    case PikumiState::kFollow:
        modelComponent_.worldTransform_.transform_.translate = Lerp(
            modelComponent_.worldTransform_.transform_.translate,
            targetFollowPos_,
            followSpeed_ * FpsCounter::deltaTime
        );
        break;

    case PikumiState::kThrown:
        // 位置更新
        modelComponent_.worldTransform_.transform_.translate += velocity_ * FpsCounter::deltaTime;
        // 減速
        velocity_ *= std::pow(dampening_, FpsCounter::deltaTime * 60.0f);

        if (velocity_.LengthSquared() < 0.1f) 
        {
            velocity_ = { 0.0f, 0.0f, 0.0f };
            state_ = PikumiState::kIdle;
        }
        break;

    case PikumiState::kIdle:
        break;
    }

    // 円形フィールド境界判定、反射処理
    Vector3 pos = modelComponent_.worldTransform_.transform_.translate;
    float distXZ = std::sqrt(pos.x * pos.x + pos.z * pos.z);

    if (distXZ > fieldRadius_ && distXZ > 0.0f)
    {
        Vector3 normal = Vector3(pos.x / distXZ, 0.0f, pos.z / distXZ);

        modelComponent_.worldTransform_.transform_.translate.x = normal.x * fieldRadius_;
        modelComponent_.worldTransform_.transform_.translate.z = normal.z * fieldRadius_;

        // 投擲中の場合の反射計算
        if (state_ == PikumiState::kThrown)
        {
            float dot = Math::Dot(velocity_, normal);
            if (dot > 0.0f)
            {
                velocity_ = velocity_ - normal * (2.0f * dot);

                // 跳ね返り時の減速
                velocity_ *= 0.9f;
            }
        }
    }

    modelComponent_.Update();

    collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());
    collider_.SetRadius(colliderRadius_);
}

void Pikumi::Draw()
{
    modelComponent_.DrawRaytracing(renderQueue_);
}

void Pikumi::OnCollisionStay(const GameEngine::CollisionResult& result)
{
    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer) && throwIgnorePlayerTimer_ > 0.0f)
    {
        return;
    }

    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer))
    {
        if (state_ == PikumiState::kThrown)
        {
            Vector3 normal = result.contactNormal;
            normal.y = 0.0f;
            if (normal.LengthSquared() > 0.0001f)
            {
                normal.Normalize();
            }

            // 法線を進行方向と逆向きに補正
            float dot = Math::Dot(velocity_, normal);
            if (dot > 0.0f)
            {
                normal = normal * -1.0f;
                dot = Math::Dot(velocity_, normal);
            }

            // 一定以上の速度で衝突している場合のみ反射と押し戻しを実行
            if (velocity_.LengthSquared() > 0.5f && dot < -0.01f)
            {
                velocity_ = velocity_ - normal * (2.0f * dot);
                velocity_ *= 0.9f; // 反射時の減速

                // 相手に進入している場合めり込み補正
                modelComponent_.worldTransform_.transform_.translate += normal * result.penetrationDepth;
            }
            else
            {
                // 遅い速度で当たった場合は Idle に遷移
                velocity_ = { 0.0f, 0.0f, 0.0f };
                state_ = PikumiState::kIdle;
            }
        }
    }
}