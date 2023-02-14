#pragma once
#include "math.hpp"
#include "chunk.hpp"
#include "chrono.hpp"

namespace game {
    struct camera {
        static constexpr f32 base_fov = 90.0f;
        static constexpr f32 sprint_fov = base_fov + 10.0f;
        static constexpr us_t fov_tween_time = 150000;

        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 look;
        f32 yaw = 0.0f;
        f32 pitch = 0.0f;

        f32 fov = base_fov;
        f32 aspect;
        f32 near_clipping_plane_distance;
        f32 far_clipping_plane_distance;

        bool update_view = false;
        bool update_look = false;
	    bool update_perspective = false;
    };

    void update_view(const camera& cam, math::matrix view);
    void update_look(camera& cam);

    void rotate_camera(camera& cam, const glm::vec2& input_vector);

    inline void update_perspective(const camera& cam, math::matrix44 perspective) {
        guPerspective(perspective, cam.fov, cam.aspect, cam.near_clipping_plane_distance, cam.far_clipping_plane_distance);
    }

    void update_needed(math::matrix view, math::matrix44 perspective, camera& cam);
    void reset_update_params(camera& cam);

    void update_camera_from_input(f32 cam_rotation_speed, camera& cam, f32 delta, u32 buttons_held);
}