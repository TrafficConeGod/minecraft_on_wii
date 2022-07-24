#pragma once
#include "math.hpp"
#include "block_util.hpp"
#include "chunk.hpp"

namespace game {
    using const_quad_verts_ref = const chunk::quad&;

    #define ADD_FACE_TEMPLATE template<typename M, void (M::*Av)(game::const_quad_verts_ref)>
    #define FACE_PARAMS M& ms_st, math::vector3u8 pos, math::vector3u8 offset_pos, math::vector2u8 uv, math::vector2u8 offset_uv
    #define SHORT_FACE_PARAMS M& ms_st, math::vector3u8 p, math::vector3u8 po, math::vector2u8 u, math::vector2u8 uo

    ADD_FACE_TEMPLATE void add_flat_front_vertices(FACE_PARAMS);
    ADD_FACE_TEMPLATE void add_flat_back_vertices(FACE_PARAMS);
    ADD_FACE_TEMPLATE void add_flat_top_vertices(FACE_PARAMS);
    ADD_FACE_TEMPLATE void add_flat_bottom_vertices(FACE_PARAMS);
    ADD_FACE_TEMPLATE void add_flat_right_vertices(FACE_PARAMS);
    ADD_FACE_TEMPLATE void add_flat_left_vertices(FACE_PARAMS);

    ADD_FACE_TEMPLATE void add_foliage_vertices(FACE_PARAMS);

    template<block::face face, typename M, void (M::*Av)(const_quad_verts_ref)>
    constexpr void add_flat_face_vertices(FACE_PARAMS) {
        call_face_func_for<face, void>(
            add_flat_front_vertices<M, Av>,
            add_flat_back_vertices<M, Av>,
            add_flat_top_vertices<M, Av>,
            add_flat_bottom_vertices<M, Av>,
            add_flat_right_vertices<M, Av>,
            add_flat_left_vertices<M, Av>,
            ms_st, pos, offset_pos, uv, offset_uv
        );
    }
}