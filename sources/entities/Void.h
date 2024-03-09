#pragma once

#include <game.h>

#include <entities/Player.h>

class Void : public Entity {
public:
    f32 height;

    void Tick() override;
private:
    void HandlePlayer(Player* player) const;
};