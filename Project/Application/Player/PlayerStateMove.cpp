#include "PlayerStateMove.h"
#include "PlayerStateIdle.h"
#include "InputCommand.h"
#include "Player.h"
#include "FPSCounter.h"

using namespace GameEngine;

void PlayerStateMove::Initialize(Player* player)
{

}

void PlayerStateMove::Update(Player* player)
{
    auto* input = player->GetInputCommand();
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    if (input->IsCommandActive("MoveUp")) { move.z += 1.0f; }
    if (input->IsCommandActive("MoveDown")) { move.z -= 1.0f; }
    if (input->IsCommandActive("MoveLeft")) { move.x -= 1.0f; }
    if (input->IsCommandActive("MoveRight")) { move.x += 1.0f; }

    // 移動入力がない場合は Idle 状態へ遷移
    if (move.LengthSquared() == 0.0f) 
    {
        player->ChangeState(std::make_unique<PlayerStateIdle>());
        return;
    }

    // 移動処理
    move.Normalize();
    player->GetWorldTransform().transform_.translate += move * player->GetMoveSpeed() * FpsCounter::deltaTime;
}