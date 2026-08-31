#pragma once
#include "IPlayerState.h"

class PlayerStateMove : public IPlayerState 
{
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
};