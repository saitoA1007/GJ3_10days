#include "PlayerStateIdle.h"
#include "PlayerStateMove.h"
#include "InputCommand.h"
#include "Player.h"

using namespace GameEngine;

void PlayerStateIdle::Initialize(Player* player)
{

}

void PlayerStateIdle::Update(Player* player)
{
    auto* input = player->GetInputCommand();
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    if (input->IsCommandActive("MoveUp")) { move.z += 1.0f; }
    if (input->IsCommandActive("MoveDown")) { move.z -= 1.0f; }
    if (input->IsCommandActive("MoveLeft")) { move.x -= 1.0f; }
    if (input->IsCommandActive("MoveRight")) { move.x += 1.0f; }

    // 移動入力がある場合に Move 状態へ遷移
    if (move.LengthSquared() > 0.0f)
    {
        player->ChangeState(std::make_unique<PlayerStateMove>());
        return;
    }

}