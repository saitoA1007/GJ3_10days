#include "Tower.h"
#include "Application/CollisionConfig.h"
#include "ImGuiManager.h"

using namespace GameEngine;

Tower::Tower(GameEngine::Model* model) : modelComponent_(model)
{
    modelComponent_.worldTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });

    // パラメータの登録
    debugParame_ = std::make_unique<GameEngine::DebugParameter>("Tower");
    debugParame_->Register("MaxHP", maxHp_, 0, "Stats");
    debugParame_->Register("ColliderRadius", colliderRadius_, 1, "Collider");
    debugParame_->Register("Scale", modelComponent_.worldTransform_.transform_.scale, 2, "Transform");

    // コライダーの初期化
    collider_.SetRadius(colliderRadius_);
    collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
    collider_.SetCollisionAttribute(kCollisionAttributeTower);
    collider_.SetCollisionMask(kCollisionAttributePlayer | kCollisionAttributePikumi | kCollisionAttributeEnemy);      

    UserData userData;
    userData.typeID = static_cast<uint32_t>(CollisionTypeID::kTower);
    userData.object = this;
    collider_.SetUserData(userData);

    collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
        this->OnCollisionEnter(result);
        });

    debugParame_->Apply();
}

void Tower::Initialize()
{
    hp_ = maxHp_;
    isDead_ = false;
    modelComponent_.worldTransform_.transform_.translate = { 0.0f, 0.0f, 0.0f };
    modelComponent_.materialData_->color = { 1.0f,0.0f,1.0f,1.0f };
}

void Tower::Update()
{
    debugParame_->ApplyIfDirty();

    modelComponent_.Update();

    collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());
    collider_.SetRadius(colliderRadius_);
}

void Tower::Draw()
{
    if (isDead_) return;
    modelComponent_.DrawRaytracing(renderQueue_);
}

void Tower::DebugUpdate()
{
#ifdef USE_IMGUI

    ImGui::Begin("Tower");

    ImGui::Text("HP: %.1f / %.1f", hp_, maxHp_);

    float hpRatio = (maxHp_ > 0.0f) ? (hp_ / maxHp_) : 0.0f;
    ImGui::ProgressBar(hpRatio, ImVec2(-1.0f, 0.0f));

    ImGui::Text("Status: %s", isDead_ ? "Destroyed" : "Alive");

    ImGui::End();

#endif
}

void Tower::TakeDamage(float damage)
{
    if (isDead_) return;

    hp_ -= damage;
    if (hp_ <= 0.0f)
    {
        hp_ = 0.0f;
        isDead_ = true;
    }
}

void Tower::OnCollisionEnter(const GameEngine::CollisionResult& result)
{
    if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy))
    {
        TakeDamage(1.0f);
    }
}