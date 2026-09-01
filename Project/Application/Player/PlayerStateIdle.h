#pragma once
#include "IPlayerState.h"

class PlayerStateIdle : public IPlayerState
{
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
};