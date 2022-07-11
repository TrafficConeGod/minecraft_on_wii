#include "chunk_mesh_generation.hpp"
#include "block_core.hpp"
#include "block_core.inl"
#include "chunk_math.hpp"
#include "chunk_core.hpp"
#include "chunk_math.hpp"
#include "dbg.hpp"
#include "face_mesh_generation.hpp"
#include "face_mesh_generation_core.hpp"
#include "face_mesh_generation_core.inl"
#include "mesh_generation.hpp"
#include "mesh_generation.inl"
#include <cstdio>

using namespace game;

template<block::face face>
static constexpr std::pair<u16, bool> get_block_lookup_neighbor(const block::lookup::neighborhood& block_nh) {
    return call_face_func_for<face, std::pair<u16, bool>>(
        [&]() { return std::make_pair(block_nh.front, block_nh.is_front_edge); },
        [&]() { return std::make_pair(block_nh.back, block_nh.is_back_edge); },
        [&]() { return std::make_pair(block_nh.top, block_nh.is_top_edge); },
        [&]() { return std::make_pair(block_nh.bottom, block_nh.is_bottom_edge); },
        [&]() { return std::make_pair(block_nh.right, block_nh.is_right_edge); },
        [&]() { return std::make_pair(block_nh.left, block_nh.is_left_edge); }
    );
}

template<typename Bf, block::face face>
static bool should_add_vertices_for_face(const block* blocks, const block* nb_blocks, u16 index, bool is_at_edge, bl_st block_state) {
    if (is_at_edge) { // We are at the edge of the chunk, so we should check the neighbor chunk.
        if (nb_blocks != nullptr) {
            return Bf::template is_face_visible_with_neighbor<face>(block_state, nb_blocks[index]);
        }

        return false;
    }
    return Bf::template is_face_visible_with_neighbor<face>(block_state, blocks[index]);
}

template<typename Bf, block::face face, typename Vf>
static void add_face_vertices_if_needed(const block* blocks, const block* nb_blocks, u16 block_nb_index, bool block_nb_is_at_face_edge, bl_st block_state, Vf& vf, math::vector3u8 block_pos) {
    if (Bf::template get_face_traits<face>(block_state).visible && should_add_vertices_for_face<Bf, face>(blocks, nb_blocks, block_nb_index, block_nb_is_at_face_edge, block_state)) {
        Bf::template add_face_vertices<face>(vf, block_pos, block_state);
    }
}

void game::update_mesh(const block::lookups& lookups, standard_quad_building_arrays& building_arrays, chunk& chunk) {
    const auto blocks = chunk.blocks.data();
    
    const auto& chunk_nh = chunk.nh;

    auto get_nb_blocks = [](chunk::const_opt_ref nb_chunk) -> const block* {
        if (nb_chunk.has_value()) {
            return nb_chunk->get().blocks.data();
        }
        return nullptr;
    };

    auto front_nb_blocks = get_nb_blocks(chunk_nh.front);
    auto back_nb_blocks = get_nb_blocks(chunk_nh.back);
    auto top_nb_blocks = get_nb_blocks(chunk_nh.top);
    auto bottom_nb_blocks = get_nb_blocks(chunk_nh.bottom);
    auto right_nb_blocks = get_nb_blocks(chunk_nh.right);
    auto left_nb_blocks = get_nb_blocks(chunk_nh.left);

    standard_vertex_function vf = {
        .it = { building_arrays }
    };

    standard_quad_iterators begin = { building_arrays };

    for (auto it = lookups.begin(); it != lookups.end(); it++) {
        u16 index = it - lookups.begin();
        auto& block = blocks[index];

        call_with_block_functionality(block.tp, [&]<typename Bf>() {
            if (Bf::get_block_traits(block.st).visible) {
                auto block_pos = it->position;
                auto& block_nh = it->nh;

                #define EVAL_CALL_FACE_VERTICES_IF_NEEDED(uppercase, lowercase) add_face_vertices_if_needed<Bf, block::face::uppercase>(blocks, lowercase##_nb_blocks, block_nh.lowercase, block_nh.is_##lowercase##_edge, block.st, vf, block_pos);

                EVAL_CALL_FACE_VERTICES_IF_NEEDED(FRONT, front)
                EVAL_CALL_FACE_VERTICES_IF_NEEDED(BACK, back)
                EVAL_CALL_FACE_VERTICES_IF_NEEDED(TOP, top)
                EVAL_CALL_FACE_VERTICES_IF_NEEDED(BOTTOM, bottom)
                EVAL_CALL_FACE_VERTICES_IF_NEEDED(RIGHT, right)
                EVAL_CALL_FACE_VERTICES_IF_NEEDED(LEFT, left)

                Bf::add_general_vertices(vf, block_pos, block.st);
            }
        });

        if (
            (vf.it.standard - begin.standard) > chunk::MAX_STANDARD_QUAD_COUNT ||
            (vf.it.foliage - begin.foliage) > chunk::MAX_FOLIAGE_QUAD_COUNT ||
            (vf.it.water - begin.water) > chunk::MAX_WATER_QUAD_COUNT
        ) {
            dbg::error([]() {
                printf("Chunk quad count is too high\n");
            });
        }
    }

    write_into_display_lists(begin, vf, chunk.standard_disp_list, chunk.foliage_disp_list, chunk.water_disp_list, [](auto vert_count) {
        return (
            4 + // GX_Begin
            vert_count * 3 + // GX_Position3u8
            vert_count * 2 + // GX_TexCoord2u8
		    1 // GX_End
        );
    }, [](auto& vert) {
        GX_Position3u8(vert.pos.x, vert.pos.y, vert.pos.z);
        GX_TexCoord2u8(vert.uv.x, vert.uv.y);
    });
}