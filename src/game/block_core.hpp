#pragma once
#include "block.hpp"
#include "math/box.hpp"
#include "util.hpp"
#include <optional>
#include <vector>

namespace game {
    template<block::face face, typename T>
    constexpr T get_face_offset_position(T pos) {
        call_face_func_for<face, void>(
            [&]() { pos.x += 1; },
            [&]() { pos.x -= 1; },
            [&]() { pos.y += 1; },
            [&]() { pos.y -= 1; },
            [&]() { pos.z += 1; },
            [&]() { pos.z -= 1; }
        );
        return pos;
    }

    template<block::face face>
    struct invert_face;

    template<>
    struct invert_face<block::face::FRONT> {
        static constexpr block::face value = block::face::BACK;
    };

    template<>
    struct invert_face<block::face::BACK> {
        static constexpr block::face value = block::face::FRONT;
    };

    template<>
    struct invert_face<block::face::TOP> {
        static constexpr block::face value = block::face::BOTTOM;
    };

    template<>
    struct invert_face<block::face::BOTTOM> {
        static constexpr block::face value = block::face::TOP;
    };

    template<>
    struct invert_face<block::face::RIGHT> {
        static constexpr block::face value = block::face::LEFT;
    };

    template<>
    struct invert_face<block::face::LEFT> {
        static constexpr block::face value = block::face::RIGHT;
    };

    bool does_world_position_collide_with_block(const glm::vec3& world_position, const block& block, const glm::vec3& world_block_position);
    std::optional<math::box> get_box_that_collides_with_world_position(const glm::vec3& world_position, const block& block, const glm::vec3& world_block_position);
}