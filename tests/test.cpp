#include <iostream>

#define GOOD_RANDOM_IMPL
#include "ecs.h"

struct Pos
{
    size_t x; 
    size_t y;
};

struct Velocity
{
    size_t x;   
    size_t y;
};

void update_pos(uuids::uuid entity, Pos* pos, Velocity* vel) 
{
    pos->x += vel->x; 
    pos->y += vel->y; 
}

void print_pos(uuids::uuid entity, Pos* pos, Velocity* vel)
{
    std::cout << uuids::to_string(entity) << " x: " << pos->x << " y: " << pos->y << " vel x: " << vel->x << " vel y: " << vel->y << std::endl; 
}

int main()
{
    ECSManager manager;
    
    Pos pos{1, 0};
    Velocity velocity{3, 2};

    uuids::uuid entity = manager.gen_uuid();
    manager.add_component(entity, std::move(pos));
    manager.add_component(entity, std::move(velocity));

    auto view = manager.get_view<Pos, Velocity>();
    view.execute(update_pos);
    view.execute(print_pos);
}

