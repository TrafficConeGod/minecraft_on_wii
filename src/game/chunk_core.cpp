#include "chunk_core.hpp"
#include "chunk_math.hpp"
#include "chunk_mesh_generation.hpp"
#include "common.hpp"
#include "glm/gtc/noise.hpp"
#include <algorithm>

using namespace game;
using math::get_noise_at;

static f32 get_biome_value(glm::vec2 position) {
    position /= 256.0f;

    return (
        (get_noise_at(position)) +
        (get_noise_at(position * 3.0f) * 0.5f)
    );
}

static f32 get_plains_height(glm::vec2 position, f32 biome_value) {
    position /= 32.0f;

    return (1.0f - biome_value) * (
        (get_noise_at(position) * 0.1f) +
        (get_noise_at(position * 3.0f) * 0.04f)
    );
}

static f32 get_hills_height(glm::vec2 position, f32 biome_value) {
    position /= 64.0f;

    return biome_value * (
        (get_noise_at(position) * 0.4f) +
        (get_noise_at(position * 3.0f) * 0.2f) +
        (get_noise_at(position * 6.0f) * 0.1f)
    );
}

static f32 get_tallgrass_value(glm::vec2 position) {
    position /= 2.0f;

    return (
        (get_noise_at(position) * 0.5f) + 
        (get_noise_at(position * 2.0f) * 1.0f)
    );
}

static void generate_high_blocks(chunk& chunk, const math::vector3s32& chunk_pos) {
    std::fill(chunk.blocks.begin(), chunk.blocks.end(), block{ .tp = block::type::AIR });
}

static void generate_middle_blocks(chunk& chunk, const math::vector3s32& chunk_pos) {
    auto blocks = chunk.blocks.data();

    for (s32 x = 0; x < chunk::SIZE; x++) {
        for (s32 z = 0; z < chunk::SIZE; z++) {
            f32 world_x = game::get_world_coord_from_block_position(x, chunk_pos.x);
            f32 world_z = game::get_world_coord_from_block_position(z, chunk_pos.z);
            glm::vec2 noise_pos = { world_x, world_z };

            auto plains_height = get_noise_at(noise_pos / 32.0f);
            
            auto tallgrass_value = get_tallgrass_value(noise_pos);

            s32 generated_height = (plains_height * 12) + 1;

            for (s32 y = 0; y < chunk::SIZE; y++) {
                auto world_height = game::get_world_coord_from_block_position(y, chunk_pos.y);
                auto index = game::get_index_from_position<std::size_t>(math::vector3s32{x, y, z});

                auto& block = blocks[index];

                if (world_height < generated_height) {
                    if (world_height < (generated_height - 2)) {
                        block = { .tp = block::type::STONE };
                    } else {
                        block = { .tp = block::type::DIRT };
                    }
                } else if (world_height == generated_height) {
                    block = { .tp = block::type::GRASS };
                } else {
                    if (world_height < 5) {
                        block = { .tp = block::type::WATER };
                    } else if (world_height == (generated_height + 1) && tallgrass_value > 0.97f) {
                        block = { .tp = block::type::TALL_GRASS };
                    } else {
                        block = { .tp = block::type::AIR };
                    }
                }
            }
        }
    }
}

static void generate_low_blocks(chunk& chunk, const math::vector3s32& chunk_pos) {
    std::fill(chunk.blocks.begin(), chunk.blocks.end(), block{ .tp = block::type::STONE });
}

void game::generate_blocks(chunk& chunk, const math::vector3s32& chunk_pos) {
    if (chunk_pos.y > 0) {
        generate_high_blocks(chunk, chunk_pos);
    } else if (chunk_pos.y < 0) {
        generate_low_blocks(chunk, chunk_pos);
    } else {
        generate_middle_blocks(chunk, chunk_pos);
    }
}

template<block::face face>
static chunk::opt_ref get_neighbor_from_map(chunk::map& chunks, const math::vector3s32& pos) {
    math::vector3s32 offset_pos = get_face_offset_position<face>(pos);
    auto it = chunks.find(offset_pos);
    if (it != chunks.end()) {
        return it->second;
    } else {
        return {};
    }
}

void game::update_chunk_neighborhood(chunk::map& chunks, const math::vector3s32& pos, chunk& chunk) {
    chunk.nh = {
        .front = get_neighbor_from_map<block::face::FRONT>(chunks, pos),
        .back = get_neighbor_from_map<block::face::BACK>(chunks, pos),
        .top = get_neighbor_from_map<block::face::TOP>(chunks, pos),
        .bottom = get_neighbor_from_map<block::face::BOTTOM>(chunks, pos),
        .right = get_neighbor_from_map<block::face::RIGHT>(chunks, pos),
        .left = get_neighbor_from_map<block::face::LEFT>(chunks, pos),
    };
}

void game::add_important_chunk_mesh_update(chunk& chunk, const math::vector3s32& pos) {
    chunk.update_mesh_important = true;
    call_func_on_each_face<void>([&chunk, &pos]<block::face face>() {
        if (is_block_position_at_face_edge<face>(pos)) {
            auto nb_chunk = get_neighbor<face>(chunk.nh);
            if (nb_chunk.has_value()) {
                nb_chunk->get().update_mesh_important = true;
            }
        }
    });
}

void game::add_chunk_mesh_neighborhood_update_to_neighbors(chunk& chunk) {
    call_func_on_each_face<void>([&chunk]<block::face face>() {
        auto nb_chunk = get_neighbor<face>(chunk.nh);
        if (nb_chunk.has_value()) {
            nb_chunk->get().update_mesh_unimportant = true;
            nb_chunk->get().update_neighborhood = true;
        }
    });
}

void game::add_chunk_neighborhood_update_to_neighbors(chunk& chunk) {
    call_func_on_each_face<void>([&chunk]<block::face face>() {
        auto nb_chunk = get_neighbor<face>(chunk.nh);
        if (nb_chunk.has_value()) {
            nb_chunk->get().update_neighborhood = true;
        }
    });
}

void game::update_chunks(const block::neighborhood_lookups& lookups, standard_quad_building_arrays& building_arrays, chunk::map& chunks, chrono::us& total_mesh_gen_us) {
    for (auto& [ pos, chunk ] : chunks) {
        if (chunk.update_neighborhood) {
            chunk.update_neighborhood = false;
            update_chunk_neighborhood(chunks, pos, chunk);
        }
    }

    bool did_important_mesh_update = false;
    for (auto& [ pos, chunk ] : chunks) {
        if (chunk.update_mesh_important) {
            did_important_mesh_update = true;
            chunk.update_mesh_important = false;
            chunk.update_mesh_unimportant = false;
            auto start_us = chrono::get_current_us();
            update_mesh(lookups, building_arrays, chunk);
            total_mesh_gen_us += chrono::get_current_us() - start_us;
        }
    }

    if (!did_important_mesh_update) {
        for (auto& [ pos, chunk ] : chunks) {
            if (chunk.update_mesh_unimportant) {
                chunk.update_mesh_important = false;
                chunk.update_mesh_unimportant = false;
                auto start_us = chrono::get_current_us();
                update_mesh(lookups, building_arrays, chunk);
                total_mesh_gen_us += chrono::get_current_us() - start_us;
                break;
            }
        }
    }
}