#include "Pikumi.h"

#include <random>

#include "FPSCounter.h"
#include "EasingManager.h"
#include "Application/CollisionConfig.h"
#include "Application/Field/ImpactDetectionEffect.h"
using namespace GameEngine;

Pikumi::Pikumi(GameEngine::Model* model, ImpactDetectionEffect* impactDetectionEffect) : modelComponent_(model)
{
    impactDetectionEffect_ = impactDetectionEffect;
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
    collider_.SetCollisionMask(kCollisionAttributeEnemy | kCollisionAttributePlayer | kCollisionAttributePikumi);

    UserData userData;
    userData.typeID = static_cast<uint32_t>(CollisionTypeID::kPikumi);
    userData.object = this;
    collider_.SetUserData(userData);

    collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
        this->OnCollisionEnter(result);
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

    // 投げた直後プレイヤーとの衝突処理を無視
    throwIgnorePlayerTimer_ = 0.1f;

    // 投げられた直後は Pikumi 同士の当たり判定を無効化
    isPikumiCollisionEnabled_ = false;
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
    {
        // 基本の目標位置への移動
        Vector3 newPos = Lerp(
            modelComponent_.worldTransform_.transform_.translate,
            targetFollowPos_,
            followSpeed_ * FpsCounter::deltaTime
        );

        // ★ Pikumi同士の重なり防止（押し出し力）
        // ※Playerクラス側からpikumis_のリストを参照させるか、簡易的に処理
        modelComponent_.worldTransform_.transform_.translate = newPos;
        break;
    }
    case PikumiState::kThrown:
        modelComponent_.worldTransform_.transform_.translate += velocity_ * FpsCounter::deltaTime;
        velocity_ *= std::pow(dampening_, FpsCounter::deltaTime * 60.0f);

        if (velocity_.LengthSquared() < 1.0f)
        {
            velocity_ = { 0.0f, 0.0f, 0.0f };
            state_ = PikumiState::kIdle;
        }
        break;

    case PikumiState::kIdle:
        break;
    }

    // アニメーションの更新
    UpdateAnimation();

    // 円形フィールド境界判定、反射処理
    Vector3 pos = modelComponent_.worldTransform_.transform_.translate;
    float distXZ = std::sqrt(pos.x * pos.x + pos.z * pos.z);

    if (distXZ > fieldRadius_ && distXZ > 0.0f)
    {
        Vector3 normal = Vector3(pos.x / distXZ, 0.0f, pos.z / distXZ);

        modelComponent_.worldTransform_.transform_.translate.x = normal.x * fieldRadius_;
        modelComponent_.worldTransform_.transform_.translate.z = normal.z * fieldRadius_;

        isPikumiCollisionEnabled_ = true;

        if (state_ == PikumiState::kThrown)
        {
            float dot = Math::Dot(velocity_, normal);
            if (dot > 0.0f)
            {
                velocity_ = velocity_ - normal * (2.0f * dot);
                velocity_ *= 0.9f;
            }
        }
    }

    modelComponent_.Update();

    // コライダーは地面位置を維持
    Vector3 colliderPos = modelComponent_.worldTransform_.GetWorldPosition();
    colliderPos.y -= animOffsetY_;
    collider_.SetWorldPosition(colliderPos);
    collider_.SetRadius(colliderRadius_);
}

void Pikumi::UpdateAnimation()
{
    if (state_ == PikumiState::kFollow)
    {
        float speedMultiplier = 0.85f + std::fmod(seed_, 0.3f);
        animTimer_ += FpsCounter::deltaTime * jumpFrequency_ * speedMultiplier;

        float bounce = std::abs(std::sin(animTimer_ + seed_));

        animOffsetY_ = bounce * jumpHeight_;

        float factor = (bounce - 0.5f) * 2.0f;
        Vector3 currentScale;
        currentScale.x = baseScale_.x * (1.0f - factor * squashStretchAmount_ * 0.5f);
        currentScale.y = baseScale_.y * (1.0f + factor * squashStretchAmount_);
        currentScale.z = baseScale_.z * (1.0f - factor * squashStretchAmount_ * 0.5f);

        modelComponent_.worldTransform_.transform_.scale = currentScale;
        modelComponent_.worldTransform_.transform_.translate.y = animOffsetY_;
    }
    else
    {
        animTimer_ = 0.0f;
        animOffsetY_ = 0.0f;
        modelComponent_.worldTransform_.transform_.scale = baseScale_;
        modelComponent_.worldTransform_.transform_.translate.y = 0.0f;
    }
}

void Pikumi::SetHighlight(bool enable)
{
    isHighlighted_ = enable;
    if (isHighlighted_)
    {
        // ハイライト時
        modelComponent_.materialData_->color = Vector4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        // 通常時
        modelComponent_.materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Pikumi::Draw()
{
    modelComponent_.DrawRaytracing(renderQueue_);
}

void Pikumi::OnCollisionEnter(const GameEngine::CollisionResult& result)
{
    // 投げ直後の Player 無視処理
    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer) && throwIgnorePlayerTimer_ > 0.0f)
    {
        return;
    }

    // Pikumi 同士の判定条件チェック
    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPikumi) && !isPikumiCollisionEnabled_)
    {
        return;
    }
    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kTower))
    {
        isPikumiCollisionEnabled_ = true;
    }

    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPikumi) ||
        result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kTower))
    {
        if (state_ == PikumiState::kThrown)
        {
            // 衝突演出
            if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy))
            {
                impactDetectionEffect_->ApplayImpact(modelComponent_.worldTransform_.transform_.translate, 50.0f);
            }

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