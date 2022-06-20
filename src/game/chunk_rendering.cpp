#include "chunk_rendering.hpp"
#include "gfx.hpp"

using namespace game;

void game::init_chunk_drawing() {
	// set number of rasterized color channels
	GX_SetNumChans(1);

	GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetChanAmbColor(GX_COLOR0A0, { 0x80, 0x80, 0x80, 0xff });
	GX_SetChanMatColor(GX_COLOR0A0, { 0x80, 0x80, 0x80, 0xff });

	//set number of textures to generate
	GX_SetNumTexGens(1);

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	//

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	// GX_VTXFMT0 is for standard cube geometry
	
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0);
	// Since the fractional size of the fixed point number is 4, it is equivalent to 1 unit = 16 pixels
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 4);
}

void game::draw_chunk(const chunk& chunk) {
    // load the modelview matrix into matrix memory
    chunk.tf.load(GX_PNMTX3);

    chunk.disp_list.call();
}

void game::draw_chunks(const gfx::texture& chunk_tex, const math::matrix view, const camera& cam, chunk::map& chunks) {
	game::init_chunk_drawing();
	gfx::load(chunk_tex);
	if (cam.update_view) {
		for (auto& [ pos, chunk ] : chunks) {
			chunk.tf.update_model_view(view);
			game::draw_chunk(chunk);
		}
	} else {
		for (auto& [ pos, chunk ] : chunks) {
			game::draw_chunk(chunk);
		}
	}
}