#pragma once
#include "block_raycast.hpp"
#include "logic.hpp"
#include "game_math.hpp"
#include <optional>

void block_selection_init(void);
void block_selection_update(Mtx view);
void block_selection_draw(us_t now);
void block_selection_handle_raycast(Mtx view, const block_raycast_wrap_t& raycast);