/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pgraph.h"
#include "pgraph_mthd.h"
#include "pgraph_class.h"
#include "nva.h"

namespace hwtest {
namespace pgraph {

static void adjust_orig_idx(struct pgraph_state *state) {
	insrt(state->idx_state_a, 20, 4, 0);
	insrt(state->idx_state_b, 16, 5, 0);
	insrt(state->idx_state_b, 24, 5, 0);
	// XXX
	state->debug[3] &= 0xefffffff;
}

static void adjust_orig_bundle(struct pgraph_state *state) {
	state->surf_unk800 = 0;
	state->debug[6] &= 0xffcfffff;
	adjust_orig_idx(state);
}

static void pgraph_kelvin_check_err19(struct pgraph_state *state) {
	if (extr(state->kelvin_unkf5c, 4, 1) && extr(state->debug[3], 3, 1) && (pgraph_class(state) & 0xff) == 0x97)
		nv04_pgraph_blowup(state, 0x80000);
}

static void pgraph_kelvin_check_err18(struct pgraph_state *state) {
	if (extr(state->kelvin_unkf5c, 0, 1) && (pgraph_class(state) & 0xff) == 0x97)
		nv04_pgraph_blowup(state, 0x40000);
}

static void pgraph_emu_celsius_calc_material(struct pgraph_state *state, uint32_t light_model_ambient[3], uint32_t material_factor_rgb[3]) {
	if (extr(state->kelvin_unkf5c, 21, 1)) {
		material_factor_rgb[0] = 0x3f800000;
		material_factor_rgb[1] = 0x3f800000;
		material_factor_rgb[2] = 0x3f800000;
		light_model_ambient[0] = state->kelvin_emu_light_model_ambient[0];
		light_model_ambient[1] = state->kelvin_emu_light_model_ambient[1];
		light_model_ambient[2] = state->kelvin_emu_light_model_ambient[2];
	} else if (extr(state->kelvin_unkf5c, 22, 1)) {
		material_factor_rgb[0] = state->kelvin_emu_light_model_ambient[0];
		material_factor_rgb[1] = state->kelvin_emu_light_model_ambient[1];
		material_factor_rgb[2] = state->kelvin_emu_light_model_ambient[2];
		light_model_ambient[0] = state->kelvin_emu_material_factor_rgb[0];
		light_model_ambient[1] = state->kelvin_emu_material_factor_rgb[1];
		light_model_ambient[2] = state->kelvin_emu_material_factor_rgb[2];
	} else {
		material_factor_rgb[0] = state->kelvin_emu_material_factor_rgb[0];
		material_factor_rgb[1] = state->kelvin_emu_material_factor_rgb[1];
		material_factor_rgb[2] = state->kelvin_emu_material_factor_rgb[2];
		light_model_ambient[0] = state->kelvin_emu_light_model_ambient[0];
		light_model_ambient[1] = state->kelvin_emu_light_model_ambient[1];
		light_model_ambient[2] = state->kelvin_emu_light_model_ambient[2];
	}
}

class MthdKelvinDmaTex : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 2, 1)) {
			exp.bundle_dma_tex[which] = rval | extr(pobj[0], 16, 2) << 24;
			pgraph_bundle(&exp, 0xa5 + which, exp.bundle_dma_tex[which], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdKelvinDmaTex(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDmaVtx : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!extr(pobj[0], 12, 2) && dcls != 0x30) {
			exp.intr |= 0x400;
			exp.fifo_enable = 0;
		}
		if (!exp.intr) {
			exp.bundle_dma_vtx[which] = rval;
			pgraph_bundle(&exp, 0xa7 + which, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdKelvinDmaVtx(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDmaState : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 3)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		exp.kelvin_dma_state = rval;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClip : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x8000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (which == 0)
				exp.bundle_clip_h = val;
			else
				exp.bundle_clip_v = val;
			pgraph_bundle(&exp, 0x6d + which, val, true);
		}
	}
public:
	MthdEmuCelsiusClip(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinClip : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (which == 0)
				exp.bundle_clip_h = val;
			else
				exp.bundle_clip_v = val;
			pgraph_bundle(&exp, 0x6d + which, val, true);
		}
	}
public:
	MthdKelvinClip(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdEmuCelsiusTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 3, 2, 1);
			if (rnd() & 3)
				insrt(val, 5, 2, 1);
			if (!(rnd() & 3))
				insrt(val, 7, 5, 0x19 + rnd() % 4);
			if (rnd() & 1)
				insrt(val, 20, 4, extr(val, 16, 4));
			if (rnd() & 3)
				insrt(val, 24, 3, 3);
			if (rnd() & 3)
				insrt(val, 28, 3, 3);
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 12, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mips = extr(val, 12, 4);
		int su = extr(val, 16, 4);
		int sv = extr(val, 20, 4);
		int fmt = extr(val, 7, 5);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (extr(val, 2, 1)) {
			if (su != sv)
				return false;
			if (su >= 0xa && (fmt == 6 || fmt == 7 || fmt == 0xb || fmt == 0xe || fmt == 0xf))
				return false;
			if (su >= 0xb)
				return false;
		}
		if (!extr(val, 3, 2) || extr(val, 3, 2) == 3)
			return false;
		if (!extr(val, 5, 2) || extr(val, 5, 2) == 3)
			return false;
		if (fmt == 0xd)
			return false;
		if (fmt >= 0x1d)
			return false;
		if (mips > 0xc || mips == 0)
			return false;
		if (fmt >= 0x19) {
			if (cls != 0x99)
				return false;
		}
		if (fmt >= 0x10) {
			if (extr(val, 2, 1))
				return false;
			if (extr(val, 24, 3) != 3)
				return false;
			if (extr(val, 28, 3) != 3)
				return false;
			if (mips != 1)
				return false;
		}
		if (su > 0xb || sv > 0xb)
			return false;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 3)
			return false;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			int mode = extr(val, 2, 1) ? 3 : 1;
			if (idx == 0)
				insrt(exp.bundle_tex_shader_op, 0, 3, mode);
			else
				insrt(exp.bundle_tex_shader_op, 5, 5, mode);
			pgraph_bundle(&exp, 0x67, exp.bundle_tex_shader_op, true);
			insrt(exp.bundle_tex_wrap[idx], 0, 3, extr(val, 24, 3));
			insrt(exp.bundle_tex_wrap[idx], 4, 1, extr(val, 27, 1));
			insrt(exp.bundle_tex_wrap[idx], 8, 3, extr(val, 28, 3));
			insrt(exp.bundle_tex_wrap[idx], 12, 1, extr(val, 31, 1));
			pgraph_bundle(&exp, 0x6f + idx, exp.bundle_tex_wrap[idx], true);
			int mips = extr(val, 12, 4);
			int su = extr(val, 16, 4);
			int sv = extr(val, 20, 4);
			if (mips > su && mips > sv)
				mips = std::max(su, sv) + 1;
			insrt(exp.bundle_tex_format[idx], 1, 1, extr(val, 1, 1));
			insrt(exp.bundle_tex_format[idx], 2, 1, extr(val, 2, 1));
			insrt(exp.bundle_tex_format[idx], 4, 1, extr(val, 4, 1));
			insrt(exp.bundle_tex_format[idx], 5, 1, extr(val, 6, 1));
			insrt(exp.bundle_tex_format[idx], 8, 5, extr(val, 7, 5));
			insrt(exp.bundle_tex_format[idx], 13, 2, 0);
			insrt(exp.bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.bundle_tex_format[idx], 20, 4, su);
			insrt(exp.bundle_tex_format[idx], 24, 4, sv);
			pgraph_bundle(&exp, 0x81 + idx, exp.bundle_tex_format[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		if (extr(val, 5, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_tex_control[idx] = val & 0x7fffffff;

			pgraph_bundle(&exp, 0x73 + idx, exp.bundle_tex_control[idx], true);
			insrt(exp.kelvin_xf_mode_c[1], idx * 16, 1, extr(val, 30, 1));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexUnk238 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_tex_unk238[idx] = val;
			pgraph_bundle(&exp, 0x7b + idx, exp.bundle_tex_unk238[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f800;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 16, 1))
			return false;
		if (!extr(val, 0, 16) || extr(val, 0, 16) >= 0x800)
			return false;
		if (!extr(val, 17, 15) || extr(val, 17, 15) >= 0x800)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_tex_rect[idx] = val & 0x1fff1fff;
			pgraph_bundle(&exp, 0x85 + idx, exp.bundle_tex_rect[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 11, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 3)
				insrt(val, 28, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 13, 11))
			return false;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 6)
			return false;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.bundle_tex_filter[idx], 0, 13, extr(val, 0, 13));
			insrt(exp.bundle_tex_filter[idx], 16, 4, extr(val, 24, 4));
			insrt(exp.bundle_tex_filter[idx], 20, 2, 0);
			insrt(exp.bundle_tex_filter[idx], 24, 2, extr(val, 28, 2));
			insrt(exp.bundle_tex_filter[idx], 26, 2, 0);
			pgraph_bundle(&exp, 0x7d + idx, exp.bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x3c;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x3e);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_tex_palette[idx] = val & ~0x32;
			pgraph_bundle(&exp, 0x8d + idx, exp.bundle_tex_palette[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				exp.bundle_tex_color_key[idx] = val;
				pgraph_bundle(&exp, 0x1c + idx, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val &= ~0x7f;
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x7f);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_offset[idx] = val;
			pgraph_bundle(&exp, 0x89 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 6, 2, 0);
			if (rnd() & 3)
				insrt(val, 15, 1, 0);
			if (rnd() & 3)
				insrt(val, 20, 4, 3);
			if (rnd() & 3)
				insrt(val, 24, 4, 3);
			if (rnd() & 3)
				insrt(val, 28, 4, 0);
			if (rnd() & 1)
				insrt(val, 24, 4, extr(val, 20, 4));
			if (rnd() & 3) {
				if (extr(val, 4, 2) == 2)
					insrt(val, 28, 4, 0);
				if (extr(val, 4, 2) == 1)
					insrt(val, 24, 8, 0);
			}
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 16, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		bool cube = extr(val, 2, 1);
		bool border = extr(val, 3, 1);
		int mode = extr(val, 4, 2);
		int fmt = extr(val, 8, 7);
		int mips = extr(val, 16, 4);
		int su = extr(val, 20, 4);
		int sv = extr(val, 24, 4);
		int sw = extr(val, 28, 4);
		bool rect = (fmt >= 0x10 && fmt <= 0x18) || (fmt >= 0x1b && fmt <= 0x26) || (fmt >= 0x2e && fmt <= 0x31) || (fmt >= 0x34 && fmt <= 0x37) || (fmt >= 0x3d && fmt <= 0x41);
		bool zcomp = (fmt >= 0x2a && fmt <= 0x31);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (fmt >= 0x42)
			return false;
		switch (fmt) {
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0d:
			case 0x21:
			case 0x22:
			case 0x23:
				return false;
		}
		if (rect) {
			if (cube)
				return false;
			if (mips != 1)
				return false;
			if (mode == 3)
				return false;
		}
		if (cube) {
			if (mode != 2)
				return false;
			if (zcomp)
				return false;
			if (su != sv)
				return false;
		}
		if (mode == 0) {
			return false;
		} else if (mode == 1) {
			int max = border ? 0xc : 0xb;
			if (su > max)
				return false;
			if (sv || sw)
				return false;
		} else if (mode == 2) {
			int max = border ? 0xc : 0xb;
			if (su > max || sv > max)
				return false;
			if (sw)
				return false;
		} else if (mode == 3) {
			int max = border ? 0x9 : 0x8;
			if (su > max || sv > max || sw > max)
				return false;
			if (zcomp)
				return false;
		}
		if (extr(val, 6, 2))
			return false;
		if (extr(val, 15, 1))
			return false;
		if (mips > 0xd || mips == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			int mode = extr(val, 4, 2);
			int fmt = extr(val, 8, 7);
			bool zcomp = (fmt >= 0x2a && fmt <= 0x31);
			int mips = extr(val, 16, 4);
			int su = extr(val, 20, 4);
			int sv = extr(val, 24, 4);
			int sw = extr(val, 28, 4);
			if (mips > su && mips > sv && mips > sw)
				mips = std::max(su, std::max(sv, sw)) + 1;
			insrt(exp.bundle_tex_format[idx], 1, 3, extr(val, 1, 3));
			insrt(exp.bundle_tex_format[idx], 6, 2, mode);
			insrt(exp.bundle_tex_format[idx], 8, 7, fmt);
			insrt(exp.bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.bundle_tex_format[idx], 20, 4, su);
			insrt(exp.bundle_tex_format[idx], 24, 4, sv);
			insrt(exp.bundle_tex_format[idx], 28, 4, sw);
			pgraph_bundle(&exp, 0x81 + idx, exp.bundle_tex_format[idx], true);
			insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], (idx & 1) * 16 + 2, 1, mode == 3 || zcomp);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexWrap : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01171717;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mode_s = extr(val, 0, 3);
		int mode_t = extr(val, 8, 3);
		int mode_r = extr(val, 16, 3);
		if (val & ~0x01171717)
			return false;
		if (mode_s < 1 || mode_s > 5)
			return false;
		if (mode_t < 1 || mode_t > 5)
			return false;
		if (mode_r < 1 || mode_r > 5)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_wrap[idx] = val & 0x01171717;
			pgraph_bundle(&exp, 0x6f + idx, exp.bundle_tex_wrap[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_control[idx] = val & 0x7fffffff;

			pgraph_bundle(&exp, 0x73 + idx, exp.bundle_tex_control[idx], true);
			insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], (idx & 1) * 16, 1, extr(val, 30, 1));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xffff;
			if (!(rnd() & 3)) {
				val &= 0xe00f0000;
			}
			if (!(rnd() & 3))
				val = 0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0xffff) && !!(val & 0xfff80000);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_pitch[idx] = val & 0xffff0000;
			pgraph_bundle(&exp, 0x77 + idx, exp.bundle_tex_pitch[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 3, 1);
			if (rnd() & 3)
				insrt(val, 16, 6, 1);
			if (rnd() & 3)
				insrt(val, 22, 2, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int unk = extr(val, 13, 3);
		int min = extr(val, 16, 6);
		int mag = extr(val, 24, 4);
		if (extr(val, 22, 2))
			return false;
		if (unk != 1 && unk != 2)
			return false;
		if (min < 1 || min > 7)
			return false;
		if (mag != 1 && mag != 2 && mag != 4)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_filter[idx] = val & 0xff3fffff;
			pgraph_bundle(&exp, 0x7d + idx, exp.bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f000;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (!extr(val, 0, 16) || extr(val, 0, 16) > 0x1000)
			return false;
		if (!extr(val, 16, 16) || extr(val, 16, 16) > 0x1000)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_rect[idx] = val & 0x1fff1fff;
			pgraph_bundle(&exp, 0x85 + idx, exp.bundle_tex_rect[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x32;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x32);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_palette[idx] = val & ~0x32;
			pgraph_bundle(&exp, 0x8d + idx, exp.bundle_tex_palette[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexBorderColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_border_color[idx] = val;
			pgraph_bundle(&exp, 0x03 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk10 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk10[idx] = val;
			pgraph_bundle(&exp, 0x07 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk11 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk11[idx] = val;
			pgraph_bundle(&exp, 0x0a + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk12 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk12[idx] = val;
			pgraph_bundle(&exp, 0x10 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk13 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk13[idx] = val;
			pgraph_bundle(&exp, 0x0d + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk14 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk14[idx] = val;
			pgraph_bundle(&exp, 0x16 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk15 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_tex_unk15[idx] = val;
			pgraph_bundle(&exp, 0x13 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_color_key[idx] = val;
			pgraph_bundle(&exp, 0x1c + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_in_alpha[idx] = val;
			pgraph_bundle(&exp, 0x30 + idx, exp.bundle_rc_in_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_in_color[idx] = val;
			pgraph_bundle(&exp, 0x40 + idx, exp.bundle_rc_in_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (idx == 0)
				exp.bundle_rc_factor_0[0] = val;
			else
				exp.bundle_rc_factor_1[0] = val;
			exp.bundle_rc_final_factor[idx] = val;
			pgraph_bundle(&exp, 0x20 + idx * 8, val, true);
			pgraph_bundle(&exp, 0x6b + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_out_alpha[idx] = val & 0x3cfff;
			pgraph_bundle(&exp, 0x38 + idx, exp.bundle_rc_out_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (!idx) {
			if (extr(val, 27, 3))
				return false;
		} else {
			int cnt = extr(val, 28, 2);
			if (cnt == 0 || cnt == 3)
				return false;
		}
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 9))
			return false;
		if (extr(val, 30, 2))
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval;
		rval = val & 0x3ffff;
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_out_color[idx] = rval;
			pgraph_bundle(&exp, 0x48 + idx, exp.bundle_rc_out_color[idx], true);
			if (idx) {
				insrt(exp.bundle_rc_config, 0, 4, extr(val, 28, 4));
				insrt(exp.bundle_rc_config, 8, 1, extr(val, 27, 1));
				insrt(exp.bundle_rc_config, 12, 1, 0);
				insrt(exp.bundle_rc_config, 16, 1, 0);
				pgraph_bundle(&exp, 0x50, exp.bundle_rc_config, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFinal0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_final_a = val & 0x3f3f3f3f;
			pgraph_bundle(&exp, 0x51, exp.bundle_rc_final_a, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFinal1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_rc_final_b = val & 0x3f3f3fe0;
			pgraph_bundle(&exp, 0x52, exp.bundle_rc_final_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_in_alpha[idx] = val;
			pgraph_bundle(&exp, 0x30 + idx, exp.bundle_rc_in_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_in_color[idx] = val;
			pgraph_bundle(&exp, 0x40 + idx, exp.bundle_rc_in_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactor0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_factor_0[idx] = val;
			pgraph_bundle(&exp, 0x20 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactor1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_factor_1[idx] = val;
			pgraph_bundle(&exp, 0x28 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinalFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_final_factor[idx] = val;
			pgraph_bundle(&exp, 0x6b + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_out_alpha[idx] = val & 0x3cfff;
			pgraph_bundle(&exp, 0x38 + idx, exp.bundle_rc_out_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 20, 12, 0);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 20, 12))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t rval;
		rval = val & 0xfffff;
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_out_color[idx] = rval;
			pgraph_bundle(&exp, 0x48 + idx, exp.bundle_rc_out_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1110f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x1110f)
			return false;
		if (extr(val, 0, 4) > 8)
			return false;
		if (extr(val, 0, 4) == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_config = val & 0x1110f;
			pgraph_bundle(&exp, 0x50, exp.bundle_rc_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinal0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_final_a = val & 0x3f3f3f3f;
			pgraph_bundle(&exp, 0x51, exp.bundle_rc_final_a, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinal1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.bundle_rc_final_b = val & 0x3f3f3fe0;
			pgraph_bundle(&exp, 0x52, exp.bundle_rc_final_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xceeeeefe)
			return false;
		if (extr(val, 28, 2) && cls != 0x99)
			return false;
		if (extr(val, 8, 1) && !extr(val, 16, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			insrt(exp.bundle_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.bundle_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.bundle_config_a, 30, 2, 0);
			insrt(exp.bundle_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.bundle_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.bundle_config_b, 10, 4, extr(val, 8, 4));
			insrt(exp.bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
			pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x00010007;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffefff8);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			insrt(exp.kelvin_unkf5c, 2, 1, extr(val, 0, 1));
			insrt(exp.kelvin_xf_mode_b, 30, 1, extr(val, 16, 1));
			insrt(exp.kelvin_xf_mode_b, 28, 1, extr(val, 2, 1));
			insrt(exp.kelvin_xf_mode_b, 18, 1, extr(val, 1, 1));
			int spec = 0;
			if (extr(exp.kelvin_unkf5c, 3, 1))
				spec = extr(exp.kelvin_unkf5c, 2, 1) ? 2 : 1;
			insrt(exp.kelvin_xf_mode_b, 19, 2, spec);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (val & ~0xf) {
			warn(1);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_unkf5c, 3, 1, extr(val, 3, 1));
				insrt(exp.kelvin_unkf5c, 21, 1, extr(val, 0, 1) && !extr(val, 1, 1));
				insrt(exp.kelvin_unkf5c, 22, 1, extr(val, 1, 1));
				int spec = 0;
				if (extr(exp.kelvin_unkf5c, 3, 1))
					spec = extr(exp.kelvin_unkf5c, 2, 1) ? 2 : 1;
				insrt(exp.kelvin_xf_mode_b, 19, 2, spec);
				insrt(exp.kelvin_xf_mode_b, 21, 2, extr(val, 2, 1));
				insrt(exp.kelvin_xf_mode_b, 23, 2, extr(val, 1, 1));
				insrt(exp.kelvin_xf_mode_b, 25, 2, extr(val, 0, 1) && !extr(val, 1, 1));
				uint32_t material_factor_rgb[3];
				uint32_t light_model_ambient[3];
				pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
				pgraph_ld_ltctx2(&exp, 0x430, material_factor_rgb[0], material_factor_rgb[1]);
				pgraph_kelvin_xf_mode(&exp);
				pgraph_ld_ltctx(&exp, 0x438, material_factor_rgb[2]);
				pgraph_ld_ltctx2(&exp, 0x410, light_model_ambient[0], light_model_ambient[1]);
				pgraph_ld_ltctx(&exp, 0x418, light_model_ambient[2]);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 16, 3, rv);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 21, 1, rv & 1);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				int rval = val;
				if (val == 0)
					rval = 4;
				insrt(exp.kelvin_xf_mode_a, 22, 3, rval);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xceeeeffe)
			return false;
		if (extr(val, 28, 2) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.bundle_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.bundle_config_a, 30, 2, extr(val, 28, 2));
			insrt(exp.bundle_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.bundle_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
			pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x00030001;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffcfffe);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_b, 30, 1, extr(val, 16, 1));
			insrt(exp.kelvin_xf_mode_b, 17, 1, extr(val, 17, 1));
			insrt(exp.kelvin_xf_mode_b, 18, 1, extr(val, 0, 1));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x1ffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & ~0x1ffff)
			return false;
		for (int i = 0; i < 8; i++)
			if (extr(val, i*2, 2) == 3)
				return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_b, 0, 2, extr(val, 14, 2));
			insrt(exp.kelvin_xf_mode_b, 2, 2, extr(val, 12, 2));
			insrt(exp.kelvin_xf_mode_b, 4, 2, extr(val, 10, 2));
			insrt(exp.kelvin_xf_mode_b, 6, 2, extr(val, 8, 2));
			insrt(exp.kelvin_xf_mode_b, 19, 2, extr(val, 6, 2));
			insrt(exp.kelvin_xf_mode_b, 21, 2, extr(val, 4, 2));
			insrt(exp.kelvin_xf_mode_b, 23, 2, extr(val, 2, 2));
			insrt(exp.kelvin_xf_mode_b, 25, 2, extr(val, 0, 2));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x804) {
			rv = 4;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 16, 3, rv);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 21, 1, rv & 1);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() {
		return val < 4 || val == 6;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			int rval = val & 7;
			if (rval == 6)
				rval = 4;
			insrt(exp.kelvin_xf_mode_a, 22, 3, rval);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 8, 1, val);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 19, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_fog_color, 0, 8, extr(val, 16, 8));
				insrt(exp.bundle_fog_color, 8, 8, extr(val, 8, 8));
				insrt(exp.bundle_fog_color, 16, 8, extr(val, 0, 8));
				insrt(exp.bundle_fog_color, 24, 8, extr(val, 24, 8));
				pgraph_bundle(&exp, 0x60, exp.bundle_fog_color, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_fog_color, 0, 8, extr(val, 16, 8));
			insrt(exp.bundle_fog_color, 8, 8, extr(val, 8, 8));
			insrt(exp.bundle_fog_color, 16, 8, extr(val, 0, 8));
			insrt(exp.bundle_fog_color, 24, 8, extr(val, 24, 8));
			pgraph_bundle(&exp, 0x60, exp.bundle_fog_color, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_raster, 31, 1, val);
			pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClipRectHoriz : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			uint32_t rval = (val & 0x0fff0fff) ^ 0x08000800;
			for (int i = idx; i < 8; i++) {
				exp.bundle_clip_rect_horiz[i] = rval;
			}
			pgraph_bundle(&exp, 0x91 + idx, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClipRectVert : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			uint32_t rval = (val & 0x0fff0fff) ^ 0x08000800;
			for (int i = idx; i < 8; i++) {
				exp.bundle_clip_rect_vert[i] = rval;
			}
			pgraph_bundle(&exp, 0x99 + idx, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectHoriz : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extr(val, 0, 12) <= extr(val, 16, 12);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			uint32_t rval = val & 0x0fff0fff;
			for (int i = idx; i < 8; i++) {
				exp.bundle_clip_rect_horiz[i] = rval;
			}
			pgraph_bundle(&exp, 0x91 + idx, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectVert : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extr(val, 0, 12) <= extr(val, 16, 12);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			uint32_t rval = val & 0x0fff0fff;
			for (int i = idx; i < 8; i++) {
				exp.bundle_clip_rect_vert[i] = rval;
			}
			pgraph_bundle(&exp, 0x99 + idx, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinAlphaFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 12, 1, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 3, 1, val);
				pgraph_bundle(&exp, 0x01, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinCullFaceEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 28, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthTestEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 14, 1, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDitherEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 22, 1, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightingEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_b, 31, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointParamsEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 9, 1, val);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 25, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 9, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 10, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 11, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusWeightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_a, 26, 3, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinWeightMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 7;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_a, 26, 3, val);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStencilEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_stencil_func, 0, 1, val);
				pgraph_bundle(&exp, 0x54, exp.bundle_stencil_func, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetPointEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 6, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetLineEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 7, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetFillEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 8, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinAlphaFuncFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 8, 4, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinAlphaFuncRef : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val & ~0xff)
			err |= 2;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 0, 8, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendFunc : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8000 : 0x300);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 0;
				break;
			case 1:
				rv = 1;
				break;
			case 0x300:
				rv = 2;
				break;
			case 0x301:
				rv = 3;
				break;
			case 0x302:
				rv = 4;
				break;
			case 0x303:
				rv = 5;
				break;
			case 0x304:
				rv = 6;
				break;
			case 0x305:
				rv = 7;
				break;
			case 0x306:
				rv = 8;
				break;
			case 0x307:
				rv = 9;
				break;
			case 0x308:
				rv = 0xa;
				break;
			case 0x8001:
				rv = 0xc;
				break;
			case 0x8002:
				rv = 0xd;
				break;
			case 0x8003:
				rv = 0xe;
				break;
			case 0x8004:
				rv = 0xf;
				break;
			default:
				err |= 1;
				break;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 4 + which * 4, 4, rv);
				pgraph_bundle(&exp, 0x01, exp.bundle_blend, true);
			}
		}
	}
public:
	MthdKelvinBlendFunc(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinBlendColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (extr(exp.kelvin_unkf5c, 0, 1)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				exp.bundle_blend_color = val;
				pgraph_bundle(&exp, 0x02, exp.bundle_blend_color, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendEquation : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8000 : 0x300);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv = 0;
		switch (val) {
			case 0x8006:
				rv = 0x2;
				break;
			case 0x8007:
				rv = 0x3;
				break;
			case 0x8008:
				rv = 0x4;
				break;
			case 0x800a:
				rv = 0x0;
				break;
			case 0x800b:
				rv = 0x1;
				break;
			default:
				err |= 1;
				break;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 0, 3, rv);
				pgraph_bundle(&exp, 0x01, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 16, 4, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorMask : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1010101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val & ~0x1010101)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 29, 1, extr(val, 0, 1));
				insrt(exp.bundle_config_a, 28, 1, extr(val, 8, 1));
				insrt(exp.bundle_config_a, 27, 1, extr(val, 16, 1));
				insrt(exp.bundle_config_a, 26, 1, extr(val, 24, 1));
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthWriteEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 24, 1, val);
				pgraph_bundle(&exp, 0x53, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStencilVal : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (which == 0 && val & ~0xff)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_stencil_func, 8 + 8 * which, 8, val);
				pgraph_bundle(&exp, 0x54, exp.bundle_stencil_func, true);
			}
		}
	}
public:
	MthdKelvinStencilVal(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinStencilFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_stencil_func, 4, 4, val);
				pgraph_bundle(&exp, 0x54, exp.bundle_stencil_func, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStencilOp : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x1500 : rnd() & 1 ? 0x8500 : 0x1e00);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 2;
				break;
			case 0x150a:
				rv = 6;
				break;
			case 0x1e00:
				rv = 1;
				break;
			case 0x1e01:
				rv = 3;
				break;
			case 0x1e02:
				rv = 4;
				break;
			case 0x1e03:
				rv = 5;
				break;
			case 0x8507:
				rv = 7;
				break;
			case 0x8508:
				rv = 8;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_stencil_op, 4 * which, 4, rv);
				pgraph_bundle(&exp, 0x55, exp.bundle_stencil_op, true);
			}
		}
	}
public:
	MthdKelvinStencilOp(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinShadeMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xf0ff;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1d00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		switch (val) {
			case 0x1d00:
				break;
			case 0x1d01:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 7, 1, val);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineWidth : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (val >= 0x200)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				uint32_t rv = val & 0x1ff;
				if (chipset.chipset == 0x10)
					rv &= 0xff;
				insrt(exp.bundle_raster, 12, 9, rv);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_polygon_offset_factor = val;
				pgraph_bundle(&exp, 0xaa, exp.bundle_polygon_offset_factor, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetUnits : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_polygon_offset_units = val;
				pgraph_bundle(&exp, 0xa9, exp.bundle_polygon_offset_units, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1b00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x1b00:
				rv = 1;
				break;
			case 0x1b01:
				rv = 2;
				break;
			case 0x1b02:
				rv = 0;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, which * 2, 2, rv);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
public:
	MthdKelvinPolygonMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDepthRangeNear : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return (cls & 0xff) != 0x97;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_depth_range_near = val;
				pgraph_bundle(&exp, 0xa4, exp.bundle_depth_range_near, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthRangeFar : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return (cls & 0xff) != 0x97;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_depth_range_far = val;
				pgraph_bundle(&exp, 0xa3, exp.bundle_depth_range_far, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinCullFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x400;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x404:
				rv = 1;
				break;
			case 0x405:
				rv = 2;
				break;
			case 0x408:
				rv = 3;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 21, 2, rv);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFrontFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x900;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		switch (val) {
			case 0x900:
				break;
			case 0x901:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 23, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinNormalizeEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_b, 27, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSpecularEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 5, 1, val);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
				if ((cls & 0xff) == 0x97) {
					insrt(exp.kelvin_xf_mode_b, 16, 1, val);
					pgraph_kelvin_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (val & ~0xffff)
			return false;
		if ((cls & 0xff) != 0x97) {
			bool off = false;
			for (int i = 0; i < 8; i++) {
				int mode = extr(val, i * 2, 2);
				if (off && mode != 0)
					return false;
				if (mode == 0)
					off = true;
			}
		}
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_a, 0, 16, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexGenMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8550 : rnd() & 1 ? 0x8510 : 0x2400);
			}
			if (!(rnd() & 3)) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		} else if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 0;
				break;
			case 0x2400:
				rv = 1;
				break;
			case 0x2401:
				rv = 2;
				break;
			case 0x8511:
				if (which < 3) {
					rv = 4;
				} else {
					err |= 1;
				}
				break;
			case 0x8512:
				if (which < 3) {
					rv = 5;
				} else {
					err |= 1;
				}
				break;
			case 0x855f:
				if (which < 3 && (cls & 0xff) != 0x97) {
					rv = (idx & 1) ? 6 : 0;
				} else {
					err |= 1;
				}
				break;
			case 0x2402:
				if (which < 2) {
					rv = 3;
				} else {
					err |= 1;
				}
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], 4 + (idx & 1) * 16 + which * 3, 3, rv);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
public:
	MthdKelvinTexGenMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which(which) {}
};

class MthdKelvinTexMatrixEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], 1 + (idx & 1) * 16, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTlMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (val & ~7)
			err |= 1;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_tex_shader_op, 3, 1, extr(val, 0, 1));
				insrt(exp.bundle_tex_shader_op, 30, 1, extr(val, 1, 1));
				insrt(exp.bundle_tex_shader_op, 31, 1, extr(val, 2, 1));
				pgraph_bundle(&exp, 0x67, exp.bundle_tex_shader_op, true);
				insrt(exp.kelvin_xf_mode_a, 30, 2, extr(val, 0, 1));
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & ~6);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_a, 18, 1, extr(val, 2, 1));
			insrt(exp.kelvin_xf_mode_a, 30, 2, extr(val, 0, 2));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointSize : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (val >= 0x200)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_point_size = val & 0x1ff;
				pgraph_bundle(&exp, 0x63, exp.bundle_point_size, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk3f0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if ((cls & 0xff) != 0x97)
			return val < 4;
		else
			return val < 5 || val == 0xf;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 25, 3, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk3f4 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 0, 1, val);
				pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusOldUnk3f8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_bundle(&exp, 0xad, exp.bundle_tex_shader_const_eye[2], true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogCoeff : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (idx < 2) {
					exp.bundle_fog_coeff[idx] = val;
					pgraph_bundle(&exp, 0x61 + idx, val, true);
				} else {
					pgraph_ld_ltctx2(&exp, 0x450, exp.bundle_fog_coeff[0], exp.bundle_fog_coeff[1]);
					pgraph_ld_ltctx(&exp, 0x458, val);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorLogicOpEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 16, 1, val);
				pgraph_bundle(&exp, 0x01, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorLogicOpOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x1500;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val >= 0x1500 && val < 0x1510) {
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 12, 4, val);
				pgraph_bundle(&exp, 0x01, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightTwoSideEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 24, 1, val);
				pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
				insrt(exp.kelvin_xf_mode_b, 29, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlUnk9cc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_a, 20, 1, val);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonStippleEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_raster, 4, 1, val);
			pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonStipple : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_bundle(&exp, 0x100 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinZPassCounterReset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val == 1;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_bundle(&exp, 0x1fd, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinZPassCounterEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_config_b, 20, 1, val);
			pgraph_bundle(&exp, 0x56, exp.bundle_config_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderCullMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_shader_cull_mode = val & 0xffff;
			pgraph_bundle(&exp, 0x65, exp.bundle_tex_shader_cull_mode, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderConstEye : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_shader_const_eye[idx] = val;
			pgraph_bundle(&exp, 0xab + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xfffe7;
			if ((rnd() & 3) == 0) {
				insrt(val, 5, 5, 0x11);
			}
			if ((rnd() & 3) == 0) {
				insrt(val, 10, 5, 0x11);
			} else if ((rnd() & 3) == 0) {
				insrt(val, 10, 5, 0x0b);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0xfffff)
			return false;
		int prev = 0;
		int pprev = 0;
		for (int i = 0; i < 4; i++) {
			int op = extr(val, i * 5, 5);
			if (op >= 6 && i == 0)
				return false;
			if (op >= 6 && i == 1 && (prev == 0 || prev == 5))
				return false;
			if (op == 0x11 && i == 3)
				return false;
			if (op == 8 && i < 2)
				return false;
			if (op == 8) {
				if (prev == 0 || prev == 5 || prev == 0x11)
					return false;
				if (pprev == 0 || pprev == 5 || pprev == 0x11)
					return false;
			}
			if (op == 9 || op == 0xa) {
				if (prev != 0x11)
					return false;
			}
			if (op == 0xb) {
				if (prev != 0x11)
					return false;
				if (i == 3)
					return false;
				int next = extr(val, (i + 1) * 5, 5);
				if (next != 0xc && next != 0x12)
					return false;
			}
			if (op == 0xc || op == 0xd || op == 0xe || op == 0x12) {
				if (prev != 0x11 && prev != 0x0b)
					return false;
				if (pprev != 0x11)
					return false;
			}
			if (op > 0x12)
				return false;

			pprev = prev;
			prev = op;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_shader_op, 0, 3, extr(val, 0, 3));
			insrt(exp.bundle_tex_shader_op, 5, 15, extr(val, 5, 15));
			pgraph_bundle(&exp, 0x67, exp.bundle_tex_shader_op, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderDotmapping : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x777;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x777)
			return false;
		for (int i = 0; i < 3; i++) {
			int map = extr(val, i * 4, 3);
			if (map == 5 && !extr(exp.debug[1], 6, 1))
				return false;
			if (map == 6 && !extr(exp.debug[1], 7, 1))
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_shader_misc, 0, 3, extr(val, 0, 3));
			insrt(exp.bundle_tex_shader_misc, 3, 3, extr(val, 4, 3));
			insrt(exp.bundle_tex_shader_misc, 6, 3, extr(val, 8, 3));
			pgraph_bundle(&exp, 0x66, exp.bundle_tex_shader_misc, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderPrevious : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x310001;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x310000)
			return false;
		if (extr(val, 20, 2) == 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_shader_misc, 15, 1, extr(val, 0, 1));
			insrt(exp.bundle_tex_shader_misc, 16, 1, extr(val, 16, 1));
			insrt(exp.bundle_tex_shader_misc, 20, 2, extr(val, 20, 2));
			pgraph_bundle(&exp, 0x66, exp.bundle_tex_shader_misc, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d64 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val == 0;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_unkf5c, 20, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFenceOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffc;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffc);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			exp.bundle_fence_offset = val;
			pgraph_bundle(&exp, 0x68, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthClamp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0x0333;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x111);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_raster, 5, 1, extr(val, 8, 1));
			insrt(exp.bundle_raster, 30, 1, extr(val, 0, 1));
			pgraph_bundle(&exp, 0x64, exp.bundle_raster, true);
			insrt(exp.bundle_unk0a1, 4, 1, extr(val, 4, 1));
			pgraph_bundle(&exp, 0xa1, exp.bundle_unk0a1, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinMultisample : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff0111;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffff0111);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_multisample = val & 0xffff0111;
			pgraph_bundle(&exp, 0x00, exp.bundle_multisample, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d80 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_unk0a1, 0, 1, val);
			pgraph_bundle(&exp, 0xa1, exp.bundle_unk0a1, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d84 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 4;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_unk0a1, 1, 2, val);
			pgraph_bundle(&exp, 0xa1, exp.bundle_unk0a1, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearZeta : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_zeta = val;
			pgraph_bundle(&exp, 0xa2, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_color = val;
			pgraph_bundle(&exp, 0x1b, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearHv : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x10001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x0fff0fff) && extr(val, 0, 16) <= extr(val, 16, 16);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_hv[which] = val & 0x0fff0fff;
			pgraph_bundle(&exp, 0x19 + which, val & 0x0fff0fff, true);
		}
	}
public:
	MthdKelvinClearHv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinUnk1e68 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_unk06a = val;
				pgraph_bundle(&exp, 0x6a, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexZcomp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 8;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_tex_zcomp = val & 7;
				pgraph_bundle(&exp, 0x69, val & 7, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1e98 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_a, 29, 1, val);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramLoadPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 0x88;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_load_pos, 0, 8, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramStartPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 0x88;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_mode_b, 8, 8, val);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlParamLoadPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 0xc0;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.kelvin_xf_load_pos, 8, 8, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinXfCtx : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_ld_xfctx(&exp, (which << 4) + (idx << 2), val);
			}
		}
	}
public:
	MthdKelvinXfCtx(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4), which(which) {}
};

class MthdKelvinXfCtx3 : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (idx < 2)
					pgraph_ld_xfctx(&exp, (which << 4) + (idx << 2), val);
				else
					pgraph_ld_xfctx2(&exp, (which << 4) + (idx << 2), val, val);
			}
		}
	}
public:
	MthdKelvinXfCtx3(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinXfCtxFree : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfctx(&exp, (which << 4) + (idx << 2), val);
		}
	}
public:
	MthdKelvinXfCtxFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4), which(which) {}
};

class MthdKelvinMatrix : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_ld_xfctx(&exp, (which << 4) + (idx << 2), val);
			}
		}
	}
public:
	MthdKelvinMatrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4), which(which) {}
};

class MthdKelvinLtCtx : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_ld_ltctx(&exp, which << 4 | idx << 2, val);
			}
		}
	}
public:
	MthdKelvinLtCtx(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinLtCtxFree : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			pgraph_ld_ltctx(&exp, which << 4 | idx << 2, val);
		}
	}
public:
	MthdKelvinLtCtxFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinLtCtxNew : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_ld_ltctx(&exp, which << 4 | idx << 2, val);
		}
	}
public:
	MthdKelvinLtCtxNew(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinLtc : public SingleMthdTest {
	int space, which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_ld_ltc(&exp, space, which << 4, val);
			}
		}
	}
public:
	MthdKelvinLtc(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int space, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), space(space), which(which) {}
};

class MthdKelvinLtcFree : public SingleMthdTest {
	int space, which;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			pgraph_ld_ltc(&exp, space, which << 4, val);
		}
	}
public:
	MthdKelvinLtcFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int space, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), space(space), which(which) {}
};

class MthdEmuCelsiusMaterialFactorRgb : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		exp.kelvin_emu_material_factor_rgb[idx] = val;
		uint32_t err = 0;
		if (extr(exp.kelvin_unkf5c, 0, 1) && cls != 0x96)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (idx == 2) {
					pgraph_ld_ltctx2(&exp, 0x430, exp.kelvin_emu_material_factor_rgb[0], exp.kelvin_emu_material_factor_rgb[1]);
					pgraph_ld_ltctx(&exp, 0x438, exp.kelvin_emu_material_factor_rgb[2]);
					uint32_t material_factor_rgb[3];
					uint32_t light_model_ambient[3];
					pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
					pgraph_ld_ltctx2(&exp, 0x430, material_factor_rgb[0], material_factor_rgb[1]);
					pgraph_ld_ltctx(&exp, 0x438, material_factor_rgb[2]);
					pgraph_ld_ltctx2(&exp, 0x410, light_model_ambient[0], light_model_ambient[1]);
					pgraph_ld_ltctx(&exp, 0x418, light_model_ambient[2]);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightModelAmbient : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		exp.kelvin_emu_light_model_ambient[idx] = val;
		if (!exp.nsource) {
			if (idx == 2) {
				uint32_t material_factor_rgb[3];
				uint32_t light_model_ambient[3];
				pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
				pgraph_ld_ltctx2(&exp, 0x430, material_factor_rgb[0], material_factor_rgb[1]);
				pgraph_ld_ltctx(&exp, 0x438, material_factor_rgb[2]);
				pgraph_ld_ltctx2(&exp, 0x410, light_model_ambient[0], light_model_ambient[1]);
				pgraph_ld_ltctx(&exp, 0x418, light_model_ambient[2]);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramLoad : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		int pos = extr(exp.kelvin_xf_load_pos, 0, 8);
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pos >= 0x88)
			pgraph_state_error(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfpr(&exp, pos << 4 | (idx & 3) << 2, val);
		}
		if ((idx & 3) == 3 && pos < 0x88) {
			insrt(exp.kelvin_xf_load_pos, 0, 8, pos + 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlParamLoad : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		int pos = extr(exp.kelvin_xf_load_pos, 8, 8);
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pos >= 0xc0)
			pgraph_state_error(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfctx(&exp, pos << 4 | (idx & 3) << 2, val);
		}
		if ((idx & 3) == 3 && pos < 0xc0) {
			insrt(exp.kelvin_xf_load_pos, 8, 8, pos + 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinVtxAttrUByte : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_ld_vtx(&exp, 4, which, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrUByte(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1), which(which), num(num) {}
};

class MthdKelvinVtxAttrShort : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_ld_vtx(&exp, 5, which, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrShort(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrNShort : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_ld_vtx(&exp, 6, which, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrNShort(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrFloat : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_ld_vtx(&exp, 7, which, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrFloat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num), which(which), num(num) {}
};

std::vector<SingleMthdTest *> EmuCelsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", 4, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", 5, cls, 0x188, 1),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx", 6, cls, 0x18c, 0),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", 7, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new MthdEmuCelsiusClip(opt, rnd(), "clip_h", 10, cls, 0x200, 0),
		new MthdEmuCelsiusClip(opt, rnd(), "clip_v", 10, cls, 0x204, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", 15, cls, 0x218, 2),
		new MthdEmuCelsiusTexFormat(opt, rnd(), "tex_format", 16, cls, 0x220, 2),
		new MthdEmuCelsiusTexControl(opt, rnd(), "tex_control", 17, cls, 0x228, 2),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", 18, cls, 0x230, 2),
		new MthdEmuCelsiusTexUnk238(opt, rnd(), "tex_unk238", 19, cls, 0x238, 2),
		new MthdEmuCelsiusTexRect(opt, rnd(), "tex_rect", 20, cls, 0x240, 2),
		new MthdEmuCelsiusTexFilter(opt, rnd(), "tex_filter", 21, cls, 0x248, 2),
		new MthdEmuCelsiusTexPalette(opt, rnd(), "tex_palette", 22, cls, 0x250, 2),
		new MthdEmuCelsiusRcInAlpha(opt, rnd(), "rc_in_alpha", 23, cls, 0x260, 2),
		new MthdEmuCelsiusRcInColor(opt, rnd(), "rc_in_color", 24, cls, 0x268, 2),
		new MthdEmuCelsiusRcFactor(opt, rnd(), "rc_factor", 25, cls, 0x270, 2),
		new MthdEmuCelsiusRcOutAlpha(opt, rnd(), "rc_out_alpha", 26, cls, 0x278, 2),
		new MthdEmuCelsiusRcOutColor(opt, rnd(), "rc_out_color", 27, cls, 0x280, 2),
		new MthdEmuCelsiusRcFinal0(opt, rnd(), "rc_final_0", 28, cls, 0x288),
		new MthdEmuCelsiusRcFinal1(opt, rnd(), "rc_final_1", 29, cls, 0x28c),
		new MthdEmuCelsiusConfig(opt, rnd(), "config", 30, cls, 0x290),
		new MthdEmuCelsiusLightModel(opt, rnd(), "light_model", 31, cls, 0x294),
		new MthdEmuCelsiusLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdEmuCelsiusFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdEmuCelsiusFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdEmuCelsiusFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdEmuCelsiusTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x2ac, 2),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdEmuCelsiusClipRectHoriz(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8),
		new MthdEmuCelsiusClipRectVert(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x300),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x304),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x308),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0x30c),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x310),
		new MthdKelvinLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x314),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x318),
		new MthdKelvinPointSmoothEnable(opt, rnd(), "point_smooth_enable", -1, cls, 0x31c),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x320),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x324),
		new MthdEmuCelsiusWeightEnable(opt, rnd(), "weight_enable", -1, cls, 0x328),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x32c),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0x330),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0x334),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0x338),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x33c),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x340),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x344, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x348, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x34c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x350),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0x354),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x358),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0x35c),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x360, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x364),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x368, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x36c, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x370, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x374, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x378, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x37c),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x380),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0x384),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0x388),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x38c, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x390, 1),
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x39c),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x3a0),
		new MthdKelvinNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x3a4),
		new MthdEmuCelsiusMaterialFactorRgb(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a8, 3),
		new MthdKelvinLtcFree(opt, rnd(), "material_factor_a", -1, cls, 0x3b4, 3, 0x0c),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x3b8),
		new MthdKelvinLightEnable(opt, rnd(), "light_enable", -1, cls, 0x3bc),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x3c0, 2, 0x10, 0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x3c4, 2, 0x10, 1),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x3c8, 2, 0x10, 2),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x3cc, 2, 0x10, 3),
		new MthdKelvinTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x3e0, 2),
		new MthdEmuCelsiusTlMode(opt, rnd(), "tl_mode", -1, cls, 0x3e8),
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x3ec),
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x3f0),
		new MthdKelvinUnk3f4(opt, rnd(), "unk3f4", -1, cls, 0x3f4),
		new MthdEmuCelsiusOldUnk3f8(opt, rnd(), "old_unk3f8", -1, cls, 0x3f8),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv0", -1, cls, 0x400, 0x08),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv1", -1, cls, 0x440, 0x10),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv0", -1, cls, 0x480, 0x0c),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv1", -1, cls, 0x4c0, 0x14),
		new MthdKelvinMatrix(opt, rnd(), "matrix_proj", -1, cls, 0x500, 0x00),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx0", -1, cls, 0x540, 0x44),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx1", -1, cls, 0x580, 0x4c),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_s_plane", -1, cls, 0x600, 0x40),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_t_plane", -1, cls, 0x610, 0x41),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_r_plane", -1, cls, 0x620, 0x42),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_q_plane", -1, cls, 0x630, 0x43),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_s_plane", -1, cls, 0x640, 0x48),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_t_plane", -1, cls, 0x650, 0x49),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_r_plane", -1, cls, 0x660, 0x4a),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_q_plane", -1, cls, 0x670, 0x4b),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x680, 3),
		new MthdKelvinXfCtxFree(opt, rnd(), "fog_plane", -1, cls, 0x68c, 0x39),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_0", -1, cls, 0x6a0, 1, 0x01),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_1", -1, cls, 0x6a4, 2, 0x01),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_2", -1, cls, 0x6a8, 3, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_3", -1, cls, 0x6ac, 0, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_4", -1, cls, 0x6b0, 2, 0x03),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_5", -1, cls, 0x6b4, 2, 0x05),
		new MthdEmuCelsiusLightModelAmbient(opt, rnd(), "light_model_ambient_color", -1, cls, 0x6c4, 3),
		new MthdKelvinXfCtx(opt, rnd(), "viewport_translate", -1, cls, 0x6e8, 0x3b),
		new MthdKelvinLtCtx(opt, rnd(), "point_params_012", -1, cls, 0x6f8, 0x47),
		new MthdKelvinLtCtx(opt, rnd(), "point_params_345", -1, cls, 0x704, 0x48),
		new MthdKelvinLtc(opt, rnd(), "point_params_6", -1, cls, 0x710, 1, 0x03),
		new MthdKelvinLtc(opt, rnd(), "point_params_7", -1, cls, 0x714, 3, 0x01),
		new MthdKelvinXfCtx(opt, rnd(), "light_eye_position", -1, cls, 0x718, 0x38),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_ambient_color", -1, cls, 0x800, 0x00),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_diffuse_color", -1, cls, 0x80c, 0x01),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_specular_color", -1, cls, 0x818, 0x02),
		new MthdKelvinLtc(opt, rnd(), "light_0_local_range", -1, cls, 0x824, 1, 0x04),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_half_vector", -1, cls, 0x828, 0x03),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_direction", -1, cls, 0x834, 0x04),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_0", -1, cls, 0x840, 1, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_1", -1, cls, 0x844, 2, 0x07),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_2", -1, cls, 0x848, 3, 0x04),
		new MthdKelvinXfCtx(opt, rnd(), "light_0_spot_direction", -1, cls, 0x84c, 0x30),
		new MthdKelvinXfCtx3(opt, rnd(), "light_0_position", -1, cls, 0x85c, 0x28),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_attenuation", -1, cls, 0x868, 0x03),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_ambient_color", -1, cls, 0x880, 0x08),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_diffuse_color", -1, cls, 0x88c, 0x09),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_specular_color", -1, cls, 0x898, 0x0a),
		new MthdKelvinLtc(opt, rnd(), "light_1_local_range", -1, cls, 0x8a4, 1, 0x05),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_half_vector", -1, cls, 0x8a8, 0x0b),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_direction", -1, cls, 0x8b4, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_0", -1, cls, 0x8c0, 1, 0x0d),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_1", -1, cls, 0x8c4, 2, 0x08),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_2", -1, cls, 0x8c8, 3, 0x05),
		new MthdKelvinXfCtx(opt, rnd(), "light_1_spot_direction", -1, cls, 0x8cc, 0x31),
		new MthdKelvinXfCtx3(opt, rnd(), "light_1_position", -1, cls, 0x8dc, 0x29),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_attenuation", -1, cls, 0x8e8, 0x0b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_ambient_color", -1, cls, 0x900, 0x10),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_diffuse_color", -1, cls, 0x90c, 0x11),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_specular_color", -1, cls, 0x918, 0x12),
		new MthdKelvinLtc(opt, rnd(), "light_2_local_range", -1, cls, 0x924, 1, 0x06),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_half_vector", -1, cls, 0x928, 0x13),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_direction", -1, cls, 0x934, 0x14),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_0", -1, cls, 0x940, 1, 0x0e),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_1", -1, cls, 0x944, 2, 0x09),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_2", -1, cls, 0x948, 3, 0x06),
		new MthdKelvinXfCtx(opt, rnd(), "light_2_spot_direction", -1, cls, 0x94c, 0x32),
		new MthdKelvinXfCtx3(opt, rnd(), "light_2_position", -1, cls, 0x95c, 0x2a),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_attenuation", -1, cls, 0x968, 0x13),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_ambient_color", -1, cls, 0x980, 0x18),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_diffuse_color", -1, cls, 0x98c, 0x19),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_specular_color", -1, cls, 0x998, 0x1a),
		new MthdKelvinLtc(opt, rnd(), "light_3_local_range", -1, cls, 0x9a4, 1, 0x07),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_half_vector", -1, cls, 0x9a8, 0x1b),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_direction", -1, cls, 0x9b4, 0x1c),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_0", -1, cls, 0x9c0, 1, 0x0f),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_1", -1, cls, 0x9c4, 2, 0x0a),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_2", -1, cls, 0x9c8, 3, 0x07),
		new MthdKelvinXfCtx(opt, rnd(), "light_3_spot_direction", -1, cls, 0x9cc, 0x33),
		new MthdKelvinXfCtx3(opt, rnd(), "light_3_position", -1, cls, 0x9dc, 0x2b),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_attenuation", -1, cls, 0x9e8, 0x1b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_ambient_color", -1, cls, 0xa00, 0x20),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_diffuse_color", -1, cls, 0xa0c, 0x21),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_specular_color", -1, cls, 0xa18, 0x22),
		new MthdKelvinLtc(opt, rnd(), "light_4_local_range", -1, cls, 0xa24, 1, 0x08),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_half_vector", -1, cls, 0xa28, 0x23),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_direction", -1, cls, 0xa34, 0x24),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_0", -1, cls, 0xa40, 1, 0x10),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_1", -1, cls, 0xa44, 2, 0x0b),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_2", -1, cls, 0xa48, 3, 0x08),
		new MthdKelvinXfCtx(opt, rnd(), "light_4_spot_direction", -1, cls, 0xa4c, 0x34),
		new MthdKelvinXfCtx3(opt, rnd(), "light_4_position", -1, cls, 0xa5c, 0x2c),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_attenuation", -1, cls, 0xa68, 0x23),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_ambient_color", -1, cls, 0xa80, 0x28),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_diffuse_color", -1, cls, 0xa8c, 0x29),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_specular_color", -1, cls, 0xa98, 0x2a),
		new MthdKelvinLtc(opt, rnd(), "light_5_local_range", -1, cls, 0xaa4, 1, 0x09),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_half_vector", -1, cls, 0xaa8, 0x2b),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_direction", -1, cls, 0xab4, 0x2c),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_0", -1, cls, 0xac0, 1, 0x11),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_1", -1, cls, 0xac4, 2, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_2", -1, cls, 0xac8, 3, 0x09),
		new MthdKelvinXfCtx(opt, rnd(), "light_5_spot_direction", -1, cls, 0xacc, 0x35),
		new MthdKelvinXfCtx3(opt, rnd(), "light_5_position", -1, cls, 0xadc, 0x2d),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_attenuation", -1, cls, 0xae8, 0x2b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_ambient_color", -1, cls, 0xb00, 0x30),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_diffuse_color", -1, cls, 0xb0c, 0x31),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_specular_color", -1, cls, 0xb18, 0x32),
		new MthdKelvinLtc(opt, rnd(), "light_6_local_range", -1, cls, 0xb24, 1, 0x0a),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_half_vector", -1, cls, 0xb28, 0x33),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_direction", -1, cls, 0xb34, 0x34),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_0", -1, cls, 0xb40, 1, 0x12),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_1", -1, cls, 0xb44, 2, 0x0d),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_2", -1, cls, 0xb48, 3, 0x0a),
		new MthdKelvinXfCtx(opt, rnd(), "light_6_spot_direction", -1, cls, 0xb4c, 0x36),
		new MthdKelvinXfCtx3(opt, rnd(), "light_6_position", -1, cls, 0xb5c, 0x2e),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_attenuation", -1, cls, 0xb68, 0x33),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_ambient_color", -1, cls, 0xb80, 0x38),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_diffuse_color", -1, cls, 0xb8c, 0x39),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_specular_color", -1, cls, 0xb98, 0x3a),
		new MthdKelvinLtc(opt, rnd(), "light_7_local_range", -1, cls, 0xba4, 1, 0x0b),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_half_vector", -1, cls, 0xba8, 0x3b),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_direction", -1, cls, 0xbb4, 0x3c),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_0", -1, cls, 0xbc0, 1, 0x13),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_1", -1, cls, 0xbc4, 2, 0x0e),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_2", -1, cls, 0xbc8, 3, 0x0b),
		new MthdKelvinXfCtx(opt, rnd(), "light_7_spot_direction", -1, cls, 0xbcc, 0x37),
		new MthdKelvinXfCtx3(opt, rnd(), "light_7_position", -1, cls, 0xbdc, 0x2f),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_attenuation", -1, cls, 0xbe8, 0x3b),
		new UntestedMthd(opt, rnd(), "vtx_pos_3f", -1, cls, 0xc00, 3), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_3s", -1, cls, 0xc10, 2), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4f", -1, cls, 0xc18, 4), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4s", -1, cls, 0xc28, 2), // XXX
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_nrm_3f", -1, cls, 0xc30, 3, 0x2),
		new MthdKelvinVtxAttrNShort(opt, rnd(), "vtx_nrm_3s", -1, cls, 0xc40, 3, 0x2),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_4f", -1, cls, 0xc50, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_3f", -1, cls, 0xc60, 3, 0x3),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col0_4ub", -1, cls, 0xc6c, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_4f", -1, cls, 0xc70, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_3f", -1, cls, 0xc80, 3, 0x4),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col1_4ub", -1, cls, 0xc8c, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_2f", -1, cls, 0xc90, 2, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_2s", -1, cls, 0xc98, 2, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_4f", -1, cls, 0xca0, 4, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_4s", -1, cls, 0xcb0, 4, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_2f", -1, cls, 0xcb8, 2, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_2s", -1, cls, 0xcc0, 2, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_4f", -1, cls, 0xcc8, 4, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_4s", -1, cls, 0xcd8, 4, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_fog_1f", -1, cls, 0xce0, 1, 0x5),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_1f", -1, cls, 0xce4, 1, 0x1),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcec), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcf0, 4), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xd00, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xdfc), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xe00, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	if (cls == 0x56) {
	} else {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk114", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
			new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0xd40),
			new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0xd44),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Kelvin::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", -1, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c, 0),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0, 1),
		new MthdDmaGrobj(opt, rnd(), "dma_fence", -1, cls, 0x1a4, 0, DMA_W | DMA_FENCE),
		new MthdDmaGrobj(opt, rnd(), "dma_query", -1, cls, 0x1a8, 1, DMA_W),
		new MthdKelvinClip(opt, rnd(), "clip_h", -1, cls, 0x200, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", -1, cls, 0x204, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new MthdKelvinRcInAlpha(opt, rnd(), "rc_in_alpha", -1, cls, 0x260, 8),
		new MthdKelvinRcFinal0(opt, rnd(), "rc_final_0", -1, cls, 0x288),
		new MthdKelvinRcFinal1(opt, rnd(), "rc_final_1", -1, cls, 0x28c),
		new MthdKelvinConfig(opt, rnd(), "config", -1, cls, 0x290),
		new MthdKelvinLightModel(opt, rnd(), "light_model", -1, cls, 0x294),
		new MthdKelvinLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdKelvinFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdKelvinFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdKelvinFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdKelvinClipRectHoriz(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8),
		new MthdKelvinClipRectVert(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x300),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x304),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x308),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0x30c),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x310),
		new MthdKelvinLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x314),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x318),
		new MthdKelvinPointSmoothEnable(opt, rnd(), "point_smooth_enable", -1, cls, 0x31c),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x320),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x324),
		new MthdKelvinWeightMode(opt, rnd(), "weight_mode", -1, cls, 0x328),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x32c),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0x330),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0x334),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0x338),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x33c),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x340),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x344, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x348, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x34c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x350),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0x354),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x358),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0x35c),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x360, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x364),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x368, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x36c, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x370, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x374, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x378, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x37c),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x380),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0x384),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0x388),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x38c, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x390, 1),
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x39c),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x3a0),
		new MthdKelvinNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x3a4),
		new MthdKelvinLtCtxFree(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a8, 0x43),
		new MthdKelvinLtcFree(opt, rnd(), "material_factor_a", -1, cls, 0x3b4, 3, 0x0c),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x3b8),
		new MthdKelvinLightEnable(opt, rnd(), "light_enable", -1, cls, 0x3bc),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x3c0, 4, 0x10, 0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x3c4, 4, 0x10, 1),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x3c8, 4, 0x10, 2),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x3cc, 4, 0x10, 3),
		new MthdKelvinTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x420, 4),
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x43c),
		new MthdKelvinMatrix(opt, rnd(), "matrix_unk", -1, cls, 0x440, 0x04),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv0", -1, cls, 0x480, 0x08),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv1", -1, cls, 0x4c0, 0x10),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv2", -1, cls, 0x500, 0x18),
		new MthdKelvinMatrix(opt, rnd(), "matrix_mv3", -1, cls, 0x540, 0x20),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv0", -1, cls, 0x580, 0x0c),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv1", -1, cls, 0x5c0, 0x14),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv2", -1, cls, 0x600, 0x1c),
		new MthdKelvinMatrix(opt, rnd(), "matrix_imv3", -1, cls, 0x640, 0x24),
		new MthdKelvinMatrix(opt, rnd(), "matrix_proj", -1, cls, 0x680, 0x00),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx0", -1, cls, 0x6c0, 0x44),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx1", -1, cls, 0x700, 0x4c),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx2", -1, cls, 0x740, 0x54),
		new MthdKelvinMatrix(opt, rnd(), "matrix_tx3", -1, cls, 0x780, 0x5c),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_s_plane", -1, cls, 0x840, 0x40),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_t_plane", -1, cls, 0x850, 0x41),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_r_plane", -1, cls, 0x860, 0x42),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_0_q_plane", -1, cls, 0x870, 0x43),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_s_plane", -1, cls, 0x880, 0x48),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_t_plane", -1, cls, 0x890, 0x49),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_r_plane", -1, cls, 0x8a0, 0x4a),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_1_q_plane", -1, cls, 0x8b0, 0x4b),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_2_s_plane", -1, cls, 0x8c0, 0x50),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_2_t_plane", -1, cls, 0x8d0, 0x51),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_2_r_plane", -1, cls, 0x8e0, 0x52),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_2_q_plane", -1, cls, 0x8f0, 0x53),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_3_s_plane", -1, cls, 0x900, 0x58),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_3_t_plane", -1, cls, 0x910, 0x59),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_3_r_plane", -1, cls, 0x920, 0x5a),
		new MthdKelvinXfCtx(opt, rnd(), "tex_gen_3_q_plane", -1, cls, 0x930, 0x5b),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x9c0, 3),
		new MthdKelvinTlUnk9cc(opt, rnd(), "tl_unk9cc", -1, cls, 0x9cc),
		new MthdKelvinXfCtxFree(opt, rnd(), "fog_plane", -1, cls, 0x9d0, 0x39),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_0", -1, cls, 0x9e0, 1, 0x01),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_1", -1, cls, 0x9e4, 2, 0x01),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_2", -1, cls, 0x9e8, 3, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_3", -1, cls, 0x9ec, 0, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_4", -1, cls, 0x9f0, 2, 0x03),
		new MthdKelvinLtcFree(opt, rnd(), "material_shininess_5", -1, cls, 0x9f4, 2, 0x05),
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x9f8),
		new MthdKelvinUnk3f4(opt, rnd(), "unk3f4", -1, cls, 0x9fc),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_model_ambient_color", -1, cls, 0xa10, 0x41),
		new MthdKelvinXfCtx(opt, rnd(), "viewport_translate", -1, cls, 0xa20, 0x3b),
		new MthdKelvinLtCtx(opt, rnd(), "point_params_012", -1, cls, 0xa30, 0x47),
		new MthdKelvinLtCtx(opt, rnd(), "point_params_345", -1, cls, 0xa3c, 0x48),
		new MthdKelvinLtc(opt, rnd(), "point_params_6", -1, cls, 0xa48, 1, 0x03),
		new MthdKelvinLtc(opt, rnd(), "point_params_7", -1, cls, 0xa4c, 3, 0x01),
		new MthdKelvinXfCtx(opt, rnd(), "light_eye_position", -1, cls, 0xa50, 0x38),
		new MthdKelvinRcFactor0(opt, rnd(), "rc_factor_0", -1, cls, 0xa60, 8),
		new MthdKelvinRcFactor1(opt, rnd(), "rc_factor_1", -1, cls, 0xa80, 8),
		new MthdKelvinRcOutAlpha(opt, rnd(), "rc_out_alpha", -1, cls, 0xaa0, 8),
		new MthdKelvinRcInColor(opt, rnd(), "rc_in_color", -1, cls, 0xac0, 8),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0xae0, 4),
		new MthdKelvinXfCtx(opt, rnd(), "viewport_scale", -1, cls, 0xaf0, 0x3a),
		new MthdKelvinTlProgramLoad(opt, rnd(), "tl_program_load", -1, cls, 0xb00, 0x20),
		new MthdKelvinTlParamLoad(opt, rnd(), "tl_param_load", -1, cls, 0xb80, 0x20),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_back_ambient_color", -1, cls, 0xc00, 0x05),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_back_diffuse_color", -1, cls, 0xc0c, 0x06),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_back_specular_color", -1, cls, 0xc18, 0x07),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_back_ambient_color", -1, cls, 0xc40, 0x0d),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_back_diffuse_color", -1, cls, 0xc4c, 0x0e),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_back_specular_color", -1, cls, 0xc58, 0x0f),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_back_ambient_color", -1, cls, 0xc80, 0x15),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_back_diffuse_color", -1, cls, 0xc8c, 0x16),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_back_specular_color", -1, cls, 0xc98, 0x17),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_back_ambient_color", -1, cls, 0xcc0, 0x1d),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_back_diffuse_color", -1, cls, 0xccc, 0x1e),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_back_specular_color", -1, cls, 0xcd8, 0x1f),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_back_ambient_color", -1, cls, 0xd00, 0x25),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_back_diffuse_color", -1, cls, 0xd0c, 0x26),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_back_specular_color", -1, cls, 0xd18, 0x27),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_back_ambient_color", -1, cls, 0xd40, 0x2d),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_back_diffuse_color", -1, cls, 0xd4c, 0x2e),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_back_specular_color", -1, cls, 0xd58, 0x2f),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_back_ambient_color", -1, cls, 0xd80, 0x35),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_back_diffuse_color", -1, cls, 0xd8c, 0x36),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_back_specular_color", -1, cls, 0xd98, 0x37),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_back_ambient_color", -1, cls, 0xdc0, 0x3d),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_back_diffuse_color", -1, cls, 0xdcc, 0x3e),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_back_specular_color", -1, cls, 0xdd8, 0x3f),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_ambient_color", -1, cls, 0x1000, 0x00),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_diffuse_color", -1, cls, 0x100c, 0x01),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_0_specular_color", -1, cls, 0x1018, 0x02),
		new MthdKelvinLtc(opt, rnd(), "light_0_local_range", -1, cls, 0x1024, 1, 0x04),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_half_vector", -1, cls, 0x1028, 0x03),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_direction", -1, cls, 0x1034, 0x04),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_0", -1, cls, 0x1040, 1, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_1", -1, cls, 0x1044, 2, 0x07),
		new MthdKelvinLtc(opt, rnd(), "light_0_spot_cutoff_2", -1, cls, 0x1048, 3, 0x04),
		new MthdKelvinXfCtx(opt, rnd(), "light_0_spot_direction", -1, cls, 0x104c, 0x30),
		new MthdKelvinXfCtx3(opt, rnd(), "light_0_position", -1, cls, 0x105c, 0x28),
		new MthdKelvinLtCtx(opt, rnd(), "light_0_attenuation", -1, cls, 0x1068, 0x03),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_ambient_color", -1, cls, 0x1080, 0x08),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_diffuse_color", -1, cls, 0x108c, 0x09),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_1_specular_color", -1, cls, 0x1098, 0x0a),
		new MthdKelvinLtc(opt, rnd(), "light_1_local_range", -1, cls, 0x10a4, 1, 0x05),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_half_vector", -1, cls, 0x10a8, 0x0b),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_direction", -1, cls, 0x10b4, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_0", -1, cls, 0x10c0, 1, 0x0d),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_1", -1, cls, 0x10c4, 2, 0x08),
		new MthdKelvinLtc(opt, rnd(), "light_1_spot_cutoff_2", -1, cls, 0x10c8, 3, 0x05),
		new MthdKelvinXfCtx(opt, rnd(), "light_1_spot_direction", -1, cls, 0x10cc, 0x31),
		new MthdKelvinXfCtx3(opt, rnd(), "light_1_position", -1, cls, 0x10dc, 0x29),
		new MthdKelvinLtCtx(opt, rnd(), "light_1_attenuation", -1, cls, 0x10e8, 0x0b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_ambient_color", -1, cls, 0x1100, 0x10),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_diffuse_color", -1, cls, 0x110c, 0x11),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_2_specular_color", -1, cls, 0x1118, 0x12),
		new MthdKelvinLtc(opt, rnd(), "light_2_local_range", -1, cls, 0x1124, 1, 0x06),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_half_vector", -1, cls, 0x1128, 0x13),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_direction", -1, cls, 0x1134, 0x14),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_0", -1, cls, 0x1140, 1, 0x0e),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_1", -1, cls, 0x1144, 2, 0x09),
		new MthdKelvinLtc(opt, rnd(), "light_2_spot_cutoff_2", -1, cls, 0x1148, 3, 0x06),
		new MthdKelvinXfCtx(opt, rnd(), "light_2_spot_direction", -1, cls, 0x114c, 0x32),
		new MthdKelvinXfCtx3(opt, rnd(), "light_2_position", -1, cls, 0x115c, 0x2a),
		new MthdKelvinLtCtx(opt, rnd(), "light_2_attenuation", -1, cls, 0x1168, 0x13),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_ambient_color", -1, cls, 0x1180, 0x18),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_diffuse_color", -1, cls, 0x118c, 0x19),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_3_specular_color", -1, cls, 0x1198, 0x1a),
		new MthdKelvinLtc(opt, rnd(), "light_3_local_range", -1, cls, 0x11a4, 1, 0x07),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_half_vector", -1, cls, 0x11a8, 0x1b),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_direction", -1, cls, 0x11b4, 0x1c),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_0", -1, cls, 0x11c0, 1, 0x0f),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_1", -1, cls, 0x11c4, 2, 0x0a),
		new MthdKelvinLtc(opt, rnd(), "light_3_spot_cutoff_2", -1, cls, 0x11c8, 3, 0x07),
		new MthdKelvinXfCtx(opt, rnd(), "light_3_spot_direction", -1, cls, 0x11cc, 0x33),
		new MthdKelvinXfCtx3(opt, rnd(), "light_3_position", -1, cls, 0x11dc, 0x2b),
		new MthdKelvinLtCtx(opt, rnd(), "light_3_attenuation", -1, cls, 0x11e8, 0x1b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_ambient_color", -1, cls, 0x1200, 0x20),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_diffuse_color", -1, cls, 0x120c, 0x21),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_4_specular_color", -1, cls, 0x1218, 0x22),
		new MthdKelvinLtc(opt, rnd(), "light_4_local_range", -1, cls, 0x1224, 1, 0x08),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_half_vector", -1, cls, 0x1228, 0x23),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_direction", -1, cls, 0x1234, 0x24),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_0", -1, cls, 0x1240, 1, 0x10),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_1", -1, cls, 0x1244, 2, 0x0b),
		new MthdKelvinLtc(opt, rnd(), "light_4_spot_cutoff_2", -1, cls, 0x1248, 3, 0x08),
		new MthdKelvinXfCtx(opt, rnd(), "light_4_spot_direction", -1, cls, 0x124c, 0x34),
		new MthdKelvinXfCtx3(opt, rnd(), "light_4_position", -1, cls, 0x125c, 0x2c),
		new MthdKelvinLtCtx(opt, rnd(), "light_4_attenuation", -1, cls, 0x1268, 0x23),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_ambient_color", -1, cls, 0x1280, 0x28),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_diffuse_color", -1, cls, 0x128c, 0x29),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_5_specular_color", -1, cls, 0x1298, 0x2a),
		new MthdKelvinLtc(opt, rnd(), "light_5_local_range", -1, cls, 0x12a4, 1, 0x09),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_half_vector", -1, cls, 0x12a8, 0x2b),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_direction", -1, cls, 0x12b4, 0x2c),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_0", -1, cls, 0x12c0, 1, 0x11),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_1", -1, cls, 0x12c4, 2, 0x0c),
		new MthdKelvinLtc(opt, rnd(), "light_5_spot_cutoff_2", -1, cls, 0x12c8, 3, 0x09),
		new MthdKelvinXfCtx(opt, rnd(), "light_5_spot_direction", -1, cls, 0x12cc, 0x35),
		new MthdKelvinXfCtx3(opt, rnd(), "light_5_position", -1, cls, 0x12dc, 0x2d),
		new MthdKelvinLtCtx(opt, rnd(), "light_5_attenuation", -1, cls, 0x12e8, 0x2b),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_ambient_color", -1, cls, 0x1300, 0x30),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_diffuse_color", -1, cls, 0x130c, 0x31),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_6_specular_color", -1, cls, 0x1318, 0x32),
		new MthdKelvinLtc(opt, rnd(), "light_6_local_range", -1, cls, 0x1324, 1, 0x0a),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_half_vector", -1, cls, 0x1328, 0x33),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_direction", -1, cls, 0x1334, 0x34),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_0", -1, cls, 0x1340, 1, 0x12),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_1", -1, cls, 0x1344, 2, 0x0d),
		new MthdKelvinLtc(opt, rnd(), "light_6_spot_cutoff_2", -1, cls, 0x1348, 3, 0x0a),
		new MthdKelvinXfCtx(opt, rnd(), "light_6_spot_direction", -1, cls, 0x134c, 0x36),
		new MthdKelvinXfCtx3(opt, rnd(), "light_6_position", -1, cls, 0x135c, 0x2e),
		new MthdKelvinLtCtx(opt, rnd(), "light_6_attenuation", -1, cls, 0x1368, 0x33),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_ambient_color", -1, cls, 0x1380, 0x38),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_diffuse_color", -1, cls, 0x138c, 0x39),
		new MthdKelvinLtCtxFree(opt, rnd(), "light_7_specular_color", -1, cls, 0x1398, 0x3a),
		new MthdKelvinLtc(opt, rnd(), "light_7_local_range", -1, cls, 0x13a4, 1, 0x0b),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_half_vector", -1, cls, 0x13a8, 0x3b),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_direction", -1, cls, 0x13b4, 0x3c),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_0", -1, cls, 0x13c0, 1, 0x13),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_1", -1, cls, 0x13c4, 2, 0x0e),
		new MthdKelvinLtc(opt, rnd(), "light_7_spot_cutoff_2", -1, cls, 0x13c8, 3, 0x0b),
		new MthdKelvinXfCtx(opt, rnd(), "light_7_spot_direction", -1, cls, 0x13cc, 0x37),
		new MthdKelvinXfCtx3(opt, rnd(), "light_7_position", -1, cls, 0x13dc, 0x2f),
		new MthdKelvinLtCtx(opt, rnd(), "light_7_attenuation", -1, cls, 0x13e8, 0x3b),
		new MthdKelvinPolygonStippleEnable(opt, rnd(), "polygon_stipple_enable", -1, cls, 0x147c),
		new MthdKelvinPolygonStipple(opt, rnd(), "polygon_stipple", -1, cls, 0x1480, 0x20),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1500, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1600, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1680, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x16c0, 4), // XXX
		new MthdKelvinXfCtxFree(opt, rnd(), "xf_unk16d0", -1, cls, 0x16d0, 0x3f),
		new MthdKelvinXfCtxFree(opt, rnd(), "xf_unk16e0", -1, cls, 0x16e0, 0x3c),
		new MthdKelvinXfCtxFree(opt, rnd(), "xf_unk16f0", -1, cls, 0x16f0, 0x3d),
		new MthdKelvinXfCtxFree(opt, rnd(), "xf_unk1700", -1, cls, 0x1700, 0x3e),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1710, 4), // XXX
		new UntestedMthd(opt, rnd(), "vtxbuf_offset", -1, cls, 0x1720, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "vtxbuf_format", -1, cls, 0x1760, 0x10), // XXX
		new MthdKelvinLtCtxFree(opt, rnd(), "light_model_back_ambient_color", -1, cls, 0x17a0, 0x42),
		new MthdKelvinLtcFree(opt, rnd(), "material_factor_back_a", -1, cls, 0x17ac, 3, 0x0d),
		new MthdKelvinLtCtxFree(opt, rnd(), "material_factor_back_rgb", -1, cls, 0x17b0, 0x44),
		new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0x17bc),
		new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0x17c0),
		new MthdKelvinLightTwoSideEnable(opt, rnd(), "light_two_side_enable", -1, cls, 0x17c4),
		new MthdKelvinZPassCounterReset(opt, rnd(), "zpass_counter_reset", -1, cls, 0x17c8),
		new MthdKelvinZPassCounterEnable(opt, rnd(), "zpass_counter_enable", -1, cls, 0x17cc),
		new UntestedMthd(opt, rnd(), "zpass_counter_read", -1, cls, 0x17d0), // XXX
		new MthdKelvinLtCtxNew(opt, rnd(), "lt_unk17d4", -1, cls, 0x17d4, 0x46),
		new MthdKelvinLtCtxNew(opt, rnd(), "lt_unk17e0", -1, cls, 0x17e0, 0x40),
		new MthdKelvinLtCtxNew(opt, rnd(), "lt_unk17ec", -1, cls, 0x17ec, 0x49),
		new MthdKelvinTexShaderCullMode(opt, rnd(), "tex_shader_cull_mode", -1, cls, 0x17f8),
		new UntestedMthd(opt, rnd(), "begin", -1, cls, 0x17fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0x1800), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1808), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1810), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1818), // XXX
		new MthdKelvinTexShaderConstEye(opt, rnd(), "tex_shader_const_eye", -1, cls, 0x181c, 3),
		new UntestedMthd(opt, rnd(), "unk1828", -1, cls, 0x1828), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1880, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1900, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1a00, 0x40), // XXX
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", -1, cls, 0x1b00, 4, 0x40),
		new MthdKelvinTexFormat(opt, rnd(), "tex_format", -1, cls, 0x1b04, 4, 0x40),
		new MthdKelvinTexWrap(opt, rnd(), "tex_wrap", -1, cls, 0x1b08, 4, 0x40),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", -1, cls, 0x1b0c, 4, 0x40),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", -1, cls, 0x1b10, 4, 0x40),
		new MthdKelvinTexFilter(opt, rnd(), "tex_filter", -1, cls, 0x1b14, 4, 0x40),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", -1, cls, 0x1b1c, 4, 0x40),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", -1, cls, 0x1b20, 4, 0x40),
		new MthdKelvinTexBorderColor(opt, rnd(), "tex_border_color", -1, cls, 0x1b24, 4, 0x40),
		new MthdKelvinTexUnk10(opt, rnd(), "tex_unk10", -1, cls, 0x1b68, 3, 0x40),
		new MthdKelvinTexUnk11(opt, rnd(), "tex_unk11", -1, cls, 0x1b6c, 3, 0x40),
		new MthdKelvinTexUnk12(opt, rnd(), "tex_unk12", -1, cls, 0x1b70, 3, 0x40),
		new MthdKelvinTexUnk13(opt, rnd(), "tex_unk13", -1, cls, 0x1b74, 3, 0x40),
		new MthdKelvinTexUnk14(opt, rnd(), "tex_unk14", -1, cls, 0x1b78, 3, 0x40),
		new MthdKelvinTexUnk15(opt, rnd(), "tex_unk15", -1, cls, 0x1b7c, 3, 0x40),
		new MthdKelvinUnk1d64(opt, rnd(), "unk1d64", -1, cls, 0x1d64),
		new UntestedMthd(opt, rnd(), "unk1d68", -1, cls, 0x1d68), // XXX
		new MthdKelvinFenceOffset(opt, rnd(), "fence_offset", -1, cls, 0x1d6c),
		new UntestedMthd(opt, rnd(), "fence_write_a", -1, cls, 0x1d70), // XXX
		new UntestedMthd(opt, rnd(), "fence_write_b", -1, cls, 0x1d74), // XXX
		new MthdKelvinDepthClamp(opt, rnd(), "depth_clamp", -1, cls, 0x1d78),
		new MthdKelvinMultisample(opt, rnd(), "multisample", -1, cls, 0x1d7c),
		new MthdKelvinUnk1d80(opt, rnd(), "unk1d80", -1, cls, 0x1d80),
		new MthdKelvinUnk1d84(opt, rnd(), "unk1d84", -1, cls, 0x1d84),
		new MthdKelvinClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0x1d8c),
		new MthdKelvinClearColor(opt, rnd(), "clear_color", -1, cls, 0x1d90),
		new UntestedMthd(opt, rnd(), "clear_trigger", -1, cls, 0x1d94), // XXX
		new MthdKelvinClearHv(opt, rnd(), "clear_h", -1, cls, 0x1d98, 0),
		new MthdKelvinClearHv(opt, rnd(), "clear_v", -1, cls, 0x1d9c, 1),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1de0, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e00, 8), // XXX
		new MthdKelvinRcFinalFactor(opt, rnd(), "rc_final_factor", -1, cls, 0x1e20, 2),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_0", -1, cls, 0x1e28, 1, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_1", -1, cls, 0x1e2c, 2, 0x02),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_2", -1, cls, 0x1e30, 3, 0x03),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_3", -1, cls, 0x1e34, 0, 0x03),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_4", -1, cls, 0x1e38, 2, 0x04),
		new MthdKelvinLtcFree(opt, rnd(), "material_back_shininess_5", -1, cls, 0x1e3c, 2, 0x06),
		new MthdKelvinRcOutColor(opt, rnd(), "rc_out_color", -1, cls, 0x1e40, 8),
		new MthdKelvinRcConfig(opt, rnd(), "rc_config", -1, cls, 0x1e60),
		new MthdKelvinUnk1e68(opt, rnd(), "unk1e68", -1, cls, 0x1e68),
		new MthdKelvinTexZcomp(opt, rnd(), "tex_zcomp", -1, cls, 0x1e6c),
		new MthdKelvinTexShaderOp(opt, rnd(), "tex_shader_op", -1, cls, 0x1e70),
		new MthdKelvinTexShaderDotmapping(opt, rnd(), "tex_shader_dotmapping", -1, cls, 0x1e74),
		new MthdKelvinTexShaderPrevious(opt, rnd(), "tex_shader_previous", -1, cls, 0x1e78),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e80, 4), // XXX
		new UntestedMthd(opt, rnd(), "xf_run_program", -1, cls, 0x1e90), // XXX
		new MthdKelvinTlMode(opt, rnd(), "tl_mode", -1, cls, 0x1e94),
		new MthdKelvinUnk1e98(opt, rnd(), "unk1e98", -1, cls, 0x1e98),
		new MthdKelvinTlProgramLoadPos(opt, rnd(), "tl_program_load_pos", -1, cls, 0x1e9c),
		new MthdKelvinTlProgramStartPos(opt, rnd(), "tl_program_start_pos", -1, cls, 0x1ea0),
		new MthdKelvinTlParamLoadPos(opt, rnd(), "tl_param_load_pos", -1, cls, 0x1ea4),
	};
	if (cls == 0x597) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "dma_clipid", -1, cls, 0x1ac), // XXX
			new UntestedMthd(opt, rnd(), "dma_zcull", -1, cls, 0x1b0), // XXX
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Rankine::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", -1, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", -1, cls, 0x188), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_b", -1, cls, 0x18c, 6, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_state", -1, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_a", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0), // XXX
		new UntestedMthd(opt, rnd(), "dma_fence", -1, cls, 0x1a4), // XXX
		new UntestedMthd(opt, rnd(), "dma_query", -1, cls, 0x1a8), // XXX
		new UntestedMthd(opt, rnd(), "dma_clipid", -1, cls, 0x1ac), // XXX
		new UntestedMthd(opt, rnd(), "dma_zcull", -1, cls, 0x1b0), // XXX
		new UntestedMthd(opt, rnd(), "clip_h", -1, cls, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "clip_v", -1, cls, 0x204), // XXX
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x218, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x220, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x240, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x280, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	return res;
}

}
}
