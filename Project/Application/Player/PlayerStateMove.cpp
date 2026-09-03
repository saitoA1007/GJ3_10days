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
    const Vector2 leftStick = input->GetLeftStick();
    Vector3 move = { leftStick.x, 0.0f, leftStick.y };

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

    // 移動中フラグ
    player->SetIsMoving(true);

    // プレイヤーの向き更新
    float yaw = std::atan2f(move.x, move.z);
    player->SetCurrentYaw(yaw);
    player->GetWorldTransform().transform_.rotate.y = yaw;

    // 移動処理
    move.Normalize();
    player->GetWorldTransform().transform_.translate += move * player->GetMoveSpeed() * FpsCounter::deltaTime;
}
