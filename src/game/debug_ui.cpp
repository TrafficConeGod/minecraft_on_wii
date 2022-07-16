#include "debug_ui.hpp"
#include <string>
#include <sstream>

#include "gfx/text.inl"

using namespace game;

debug_ui::debug_ui() {
    tf.set_position(10.0f, 20.0f);
    tf.load(MAT);

    constexpr auto write_text = [](gfx::display_list& disp_list, std::string_view str, u16 y_offset) {
        gfx::write_text_into_display_list<u8>([y_offset](u16 x, u16 y, u8 u, u8 v) {
            GX_Position2u16(x, y + (CHAR_SIZE * y_offset));
            GX_TexCoord2u8(u, v);
        }, disp_list, str, CHAR_SIZE, CHAR_SIZE);
    };

    write_text(pos_prefix_disp_list, POS_PREFIX, 0);
    write_text(dir_prefix_disp_list, DIR_PREFIX, 1);
    write_text(fps_prefix_disp_list, FPS_PREFIX, 2);
    write_text(bgt_prefix_disp_list, BGT_PREFIX, 3);
    write_text(mgt_prefix_disp_list, MGT_PREFIX, 4);
}

static inline std::string to_string(const glm::vec3& v) {
    std::stringstream ss;
    ss << v.x << ' ' << v.y << ' ' << v.z;
    return ss.str();
}

void debug_ui::draw(const glm::vec3& pos, const glm::vec3& dir, chrono::us block_gen_time, chrono::us mesh_gen_time, u32 fps) const {
    GX_SetCurrentMtx(MAT);

    pos_prefix_disp_list.call();
    dir_prefix_disp_list.call();
    fps_prefix_disp_list.call();
    bgt_prefix_disp_list.call();
    mgt_prefix_disp_list.call();

    auto pos_str = to_string(pos);
    auto dir_str = to_string(dir);
    auto fps_str = std::to_string(fps);
    auto block_gen_time_str = std::to_string(block_gen_time);
    auto mesh_gen_time_str = std::to_string(mesh_gen_time);
    std::size_t vertex_count = 4 * (pos_str.size() + dir_str.size() + fps_str.size() + block_gen_time_str.size() + mesh_gen_time_str.size());
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, vertex_count);

    constexpr auto write_text = [](std::string_view str, u16 y_offset) {
        gfx::write_text_vertices<u8>([y_offset](u16 x, u16 y, u8 u, u8 v) {
            GX_Position2u16(x + PREFIX_WIDTH, y + (CHAR_SIZE * y_offset));
            GX_TexCoord2u8(u, v);
        }, str, CHAR_SIZE, CHAR_SIZE);
    };

    write_text(pos_str, 0);
    write_text(dir_str, 1);
    write_text(fps_str, 2);
    write_text(block_gen_time_str, 3);
    write_text(mesh_gen_time_str, 4);

    GX_End();
}