#pragma once
#include "SceneRegistry.h"

// 各シーン
#ifdef USE_IMGUI
#include "Application/Scene/TestScene.h"
#include "Application/Scene/ProtoScene.h"
#endif
#include "Application/Scene/GameScene.h"
//#include "Application/Scene/StageSelectScene.h"
#include "Application/Scene/TitleScene.h"

using namespace GameEngine;

/// <summary>
/// シーンを登録する
/// </summary>
/// <param name="factory"></param>
void SetupScenes(SceneRegistry& factory) {

    // 各シーンの登録
#ifdef USE_IMGUI
    factory.RegisterScene<TestScene>("Test");
    factory.RegisterScene<ProtoScene>("Proto");
#endif
    factory.RegisterScene<GameScene>("Game");
    factory.RegisterScene<TitleScene>("Title");
    //factory.RegisterScene<StageSelectScene>("StageSelect");

    // 立ち上げ時に起動するシーン
    factory.SetDefaultScene("Title");
}
