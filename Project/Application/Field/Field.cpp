#include "Field.h"
#include <numbers>
#include "FPSCounter.h"
#include "MyMath.h"
using namespace GameEngine;

Field::Field(GameEngine::Model* model, GameEngine::Model* poleModel, GameEngine::Model* circleModel, GameEngine::Model* planeModel, FieldEffect* fieldEffect)
    : modelComponent_(model), universeModel_(planeModel)
{
    fieldEffect_ = fieldEffect;

    modelComponent_.worldTransform_.Initialize({ {fieldRadius_, height_, fieldRadius_},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

    // パラメータの登録
    debugParame_ = std::make_unique<GameEngine::DebugParameter>("Field");
    debugParame_->Register("Radius", fieldRadius_, 0, "Transform");
    debugParame_->Register("Height", height_, 1, "Transform");
    debugParame_->Register("pos", modelComponent_.worldTransform_.transform_.translate, 1, "Transform");
    debugParame_->Register("ior", modelComponent_.materialData_->ior, 1, "Material");
    debugParame_->Register("metallic", modelComponent_.materialData_->metallic, 1, "Material");
    debugParame_->Register("roughness", modelComponent_.materialData_->roughness, 1, "Material");
    debugParame_->Register("color", modelComponent_.materialData_->color, 1, "Material");
    // レインボー
    debugParame_->Register("Cycle", colorCycle_, 1, "Color");
    debugParame_->Register("Offset", colorOffset_, 1, "Color");

    debugParame_->Register("radius", universeMaterial_.materialData_->radius, 1, "UniverseMaterial");
    debugParame_->Register("swirl", universeMaterial_.materialData_->swirl, 1, "UniverseMaterial");
    debugParame_->Register("scale", universeMaterial_.materialData_->scale, 1, "UniverseMaterial");
    debugParame_->Register("strength", universeMaterial_.materialData_->strength, 1, "UniverseMaterial");
    debugParame_->Register("UniversePos", universeMaterial_.materialData_->UniversePos, 1, "UniverseMaterial");
    debugParame_->Register("Pos", universeModel_.worldTransform_.transform_.translate, 1, "UniverseTransform");
    debugParame_->Register("scale", universeModel_.worldTransform_.transform_.scale, 1, "UniverseTransform");
    universeModel_.SetHitGroup(2);
    universeModel_.SetBufferMaterial(0, universeMaterial_.GetMaterialSrvIndex());

    // ポール
    poleModels_.reserve(20);
    for (uint32_t i = 0; i < 20; ++i) {
        std::unique_ptr<ModelComponent> frame = std::make_unique<ModelComponent>(poleModel);

        float rad = i * (1.0f / std::numbers::pi_v<float>);
        float radius = 20.0f;

        float x = radius * std::sinf(rad);
        float z = radius * std::cosf(rad);

        frame->worldTransform_.transform_.scale = { 1.0f,20.0f,1.0f };
        frame->worldTransform_.transform_.translate = {x,-20.0f,z};

        frame->worldTransform_.UpdateTransformMatrix();
        poleModels_.push_back(std::move(frame));
    }

    // 円
    circleModels_.reserve(8);
    for (uint32_t i = 0; i < 8; ++i) {
        std::unique_ptr<ModelComponent> circle = std::make_unique<ModelComponent>(circleModel);

        circle->worldTransform_.transform_.scale = { 20.0f,0.1f,20.0f };
        circle->worldTransform_.transform_.translate = { 0.0f,i * -5.0f,0.0f };

        circle->worldTransform_.UpdateTransformMatrix();
        circleModels_.push_back(std::move(circle));
    }

    debugParame_->Apply();

    modelComponent_.worldTransform_.transform_.scale = { fieldRadius_, height_, fieldRadius_ };
}

void Field::Initialize()
{}

void Field::Update()
{
    if (debugParame_->ApplyIfDirty())
    {
        modelComponent_.worldTransform_.transform_.scale = { fieldRadius_, height_, fieldRadius_ };
    }

    universeMaterial_.materialData_->time += FpsCounter::gameDeltaTime;

    colorTime_ += FpsCounter::gameDeltaTime;

	for (uint32_t i = 0; i < circleModels_.size(); ++i) {
		float h = colorTime_ / colorCycle_ + colorOffset_ * i;
		Vector3 rgb = Math::HSVtoRGB(h, 1.0f, 1.0f);
		circleModels_[i]->materialData_->color = { rgb.x, rgb.y, rgb.z, 1.0f };
	}

    universeModel_.Update();

    modelComponent_.Update();
}

void Field::Draw()
{
    //modelComponent_.DrawRaytracing(renderQueue_);

    // 宇宙を描画
    universeModel_.DrawCustomRaytracing(renderQueue_);

    // ポール描画
    for (auto& pole : poleModels_) {
        pole->DrawRaytracing(renderQueue_);
    }

    // 円
    for (auto& circle : circleModels_) {
        circle->DrawRaytracing(renderQueue_);
    }
}