#include "Field.h"
#include "FPSCounter.h"
#include<numbers>
using namespace GameEngine;

Field::Field(GameEngine::Model* model) : modelComponent_(model)
{
    modelComponent_.worldTransform_.Initialize({ {fieldRadius_, height_, fieldRadius_},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

    // パラメータの登録
    debugParame_ = std::make_unique<GameEngine::DebugParameter>("Field");
    debugParame_->Register("Radius", fieldRadius_, 0, "Transform");
    debugParame_->Register("Height", height_, 1, "Transform");
    debugParame_->Register("translate", modelComponent_.worldTransform_.transform_.translate, 1, "Transform");

    debugParame_->Register("radius", universeMaterial_.materialData_->radius, 1, "Material");
    debugParame_->Register("swirl", universeMaterial_.materialData_->swirl, 1, "Material");
    debugParame_->Register("scale", universeMaterial_.materialData_->scale, 1, "Material");
    debugParame_->Register("strength", universeMaterial_.materialData_->strength, 1, "Material");
    debugParame_->Register("UniversePos", universeMaterial_.materialData_->UniversePos, 1, "Material");

    debugParame_->Apply();
    modelComponent_.worldTransform_.transform_.scale = { fieldRadius_, height_, fieldRadius_ };
    modelComponent_.SetHitGroup(2);
    modelComponent_.SetBufferMaterial(0, universeMaterial_.GetMaterialSrvIndex());
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

    modelComponent_.Update();
}

void Field::Draw()
{
    modelComponent_.DrawRaytracing(renderQueue_);
}