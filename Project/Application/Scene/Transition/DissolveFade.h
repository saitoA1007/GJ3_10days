#pragma once
#include"ITransitionEffect.h"
#include "PostProcess/PostEffectData.h"

class DissolveFade : public ITransitionEffect {
public:
    DissolveFade(GameEngine::Dissolve* dissolve);

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update(float timer) override;

    // 描画処理
    void Draw() override;

    // 中間を追加
    bool IsMidTransition(float timer) const override;

    // 遷移する時間
    float GetMaxTime() override { return 1.0f; }

private:
 
    GameEngine::Dissolve* dissolve_ = nullptr;
};