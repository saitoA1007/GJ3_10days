#include "Field.h"

using namespace GameEngine;

Field::Field(GameEngine::Model* model) : modelComponent_(model)
{
    modelComponent_.worldTransform_.Initialize({ {fieldRadius_, height_, fieldRadius_},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

    // パラメータの登録
    debugParame_ = std::make_unique<GameEngine::DebugParameter>("Field");
    debugParame_->Register("Radius", fieldRadius_, 0, "Transform");
    debugParame_->Register("Height", height_, 1, "Transform");

    debugParame_->Apply();
}

void Field::Initialize()
{}

void Field::Update()
{
    if (debugParame_->ApplyIfDirty())
    {
        modelComponent_.worldTransform_.transform_.scale = { fieldRadius_, height_, fieldRadius_ };
    }

    modelComponent_.Update();
}

void Field::Draw()
{
    modelComponent_.DrawRaytracing(renderQueue_);
}