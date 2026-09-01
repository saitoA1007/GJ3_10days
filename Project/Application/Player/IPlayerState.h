#pragma once

class Player;

class IPlayerState 
{
public:
    virtual ~IPlayerState() = default;
    virtual void Initialize(Player* player) = 0;
    virtual void Update(Player* player) = 0;
};