#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <cmath>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <array>
#include "gfx.hpp"
#include "dbg.hpp"
#include "math.hpp"
#include "input.hpp"
#include <map>
#include <numeric>
#include "game/block_raycast.hpp"
#include "game/block_raycast.inl"
#include "game/camera.hpp"
#include "game/chunk_core.hpp"
#include "game/chunk_management.hpp"
#include "game/chunk_rendering.hpp"
#include "game/stored_chunk.hpp"
#include "game/logic.hpp"
#include "game/block_selection.hpp"
#include "game/cursor.hpp"
#include "game/skybox.hpp"
#include "game/character.hpp"
#include "game/rendering.hpp"
#include "common.hpp"

static constexpr f32 cam_rotation_speed = 0.15f;

static constexpr s32 chunk_generation_radius = 3;

static constexpr s32 chunk_erasure_radius = 4;

int main(int argc, char** argv) {

	gfx::console_state con;

	if (!fatInitDefault()) {
		printf("fatInitDefault() failed!\n");
		dbg::freeze();
	}

	gfx::texture chunk_tex;
	gfx::safe_load_from_file(chunk_tex, "data/textures/chunk.tpl");

	gfx::texture icons_tex;
	gfx::safe_load_from_file(icons_tex, "data/textures/icons.tpl");

	gfx::texture skybox_tex;
	gfx::safe_load_from_file(skybox_tex, "data/textures/skybox.tpl");

	input::init(con.rmode->viWidth, con.rmode->viHeight);

	gfx::draw_state draw{ {0xFF, 0xFF, 0xFF, 0xFF} };

	input::set_resolution(draw.rmode->viWidth, draw.rmode->viHeight);

	gfx::set_filtering_mode(chunk_tex, GX_NEAR, GX_NEAR);
	gfx::set_filtering_mode(icons_tex, GX_NEAR, GX_NEAR);


	gfx::load(chunk_tex, GX_TEXMAP0);
	gfx::load(icons_tex, GX_TEXMAP1);
	gfx::load(skybox_tex, GX_TEXMAP2);

	math::matrix44 perspective_2d;
	guOrtho(perspective_2d, 0, 479, 0, 639, 0, 300);
	
	math::matrix view;
	math::matrix44 perspective_3d;

	game::character character = {
		.position = { 0.0f, 0.0f, 0.0f },
		.velocity = { 0.0f, 0.0f, 0.0f }
	};

	game::camera cam = {
		.position = {0.0f, 0.0f, -10.0f},
		.up = {0.0f, 1.0f, 0.0f},
		.look = {0.0f, 0.0f, 1.0f},
		.fov = 90.0f,
		.aspect = (f32)((f32)draw.rmode->viWidth / (f32)draw.rmode->viHeight),
		.near_clipping_plane_distance = 0.1f,
		.far_clipping_plane_distance = 300.0f
	};
	character.update_camera(cam);
	std::optional<math::vector3s32> last_cam_chunk_pos;
	math::normalize(cam.look);

	game::update_look(cam);
	game::update_view(cam, view);

	game::update_perspective(cam, perspective_3d);

	bool first_frame = true;
	#ifdef PC_PORT
	u16 frame_count = 0;
	#endif

	game::chunk::map chunks;
	// This is a variable whose lifetime is bound to mesh updating functions normally. However, since it takes up quite a bit of memory, it is stored here.
	ext::data_array<game::chunk::quad> building_quads(game::chunk::MAX_QUAD_COUNT + 0x100);
	game::stored_chunk::map stored_chunks;

	input::state inp;

	// This is a variable whose lifetime is bound to the manage_chunks_around_camera function normally. However, reallocation is expensive, it is stored here.
	std::vector<math::vector3s32> chunk_positions_to_erase;

	game::skybox skybox{view, cam};
	
	game::cursor cursor;

	game::block_selection bl_sel;

    GX_SetCurrentMtx(GX_PNMTX3);

	gfx::set_z_buffer_mode(true, GX_LEQUAL, true);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetAlphaUpdate(GX_TRUE);

	for (;;) {
		auto raycast = game::get_block_raycast(chunks, cam.position, cam.position + (cam.look * 20.0f), []<typename Bf>(game::bl_st st) {
			return Bf::get_selection_boxes(st);
		});
		bl_sel.handle_raycast(view, building_quads, raycast);

		game::update_from_input(cam_rotation_speed, draw.rmode->viWidth, draw.rmode->viHeight, character, cam, chunks, cursor, raycast);
		character.apply_physics(chunks);
		character.apply_velocity();
		character.update_camera(cam);

		game::manage_chunks_around_camera(chunk_erasure_radius, chunk_generation_radius, view, cam, last_cam_chunk_pos, chunks, stored_chunks, chunk_positions_to_erase);


		game::update_needed(view, perspective_3d, cam);

		game::update_chunks(chunks, building_quads);

		GX_LoadProjectionMtx(perspective_3d, GX_PERSPECTIVE);
		skybox.update_if_needed(view, cam);
		skybox.draw();
		
		game::init_standard_rendering();
		game::draw_chunks_standard(view, cam, chunks);
		bl_sel.update_if_needed(view, cam);
		bl_sel.draw_standard(raycast);

		game::init_foliage_rendering();
		game::draw_chunks_foliage(chunks);
		bl_sel.draw_foliage(raycast);

		game::init_water_rendering();
		game::draw_chunks_water(chunks);
		bl_sel.draw_water(raycast);
		
		GX_LoadProjectionMtx(perspective_2d, GX_ORTHOGRAPHIC);
		cursor.draw();

		game::reset_update_params(cam);

		gfx::copy_framebuffer(draw.frame_buffers[draw.fb_index], true);

		gfx::draw_done();

		VIDEO_SetNextFramebuffer(draw.frame_buffers[draw.fb_index]);
		if (first_frame) {
			first_frame = false;
			VIDEO_SetBlack(FALSE);
		}
		VIDEO_Flush();
		VIDEO_WaitVSync();
		draw.fb_index ^= 1;
		
		#ifdef PC_PORT
		if (++frame_count == 1200) {
			std::exit(0);
		}
		#endif
	}
}
