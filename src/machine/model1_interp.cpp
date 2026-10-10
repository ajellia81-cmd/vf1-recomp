// Smooth motion above 1x. VF's game logic runs at 30 Hz: it builds a new display list every
// second frame, so the picture moves 30 times a second whatever the screen can show. In smooth
// mode each new list is kept (with the sky's scroll), and the picture is drawn for any moment
// between the last two: every object's matrix (its place, turn and the camera's) is blended
// with the same object's in the list before, and the sky's scroll likewise. The game itself runs
// untouched; the picture is one game frame (two video frames) behind it.
#include "model1.h"

namespace {

// An object in a list: its polygons' address, and which time that address comes in the list.
uint64_t object_key(uint32_t poly_adr, uint32_t nth) { return uint64_t(poly_adr) << 20 | nth; }

} // namespace

void model1::set_smooth(bool on)
{
	if (on == m_smooth) return;
	m_smooth = on;
	m_logic[0] = m_logic[1] = logic_frame{};
	m_shown = false;
}

// A list's matrix slots: each matrix command (0xb) and the objects drawn with it. Swapped models
// (a hand, a face) keep their slot, so slots are what is matched between two lists.
void model1::index_objects(logic_frame &lf)
{
	lf.slots.clear();
	lf.slots.push_back({});                        // objects before the first matrix
	const uint16_t *l = lf.list.data();
	auto rdi = [&](int a) { return uint32_t(l[a & 0x7fff] | l[(a + 1) & 0x7fff] << 16); };
	auto rdf = [&](int a) { uint32_t v = rdi(a); float f; memcpy(&f, &v, 4); return f; };
	int off = 0;
	for (int guard = 0; guard < 0x10000; guard++) {
		switch (rdi(off)) {
		case 0: off += 2; break;
		case 1: case 0x41:
			lf.slots.back().polys.push_back(rdi(off + 4));
			lf.objects++;
			off += 8;
			break;
		case 2: {                                  // as skip_direct
			off += 18;
			for (;;) {
				const uint32_t t = rdi(off + 2) & 3;
				if (!t) break;
				off += t == 2 ? 12 : 20;
			}
			off += 4;
			break;
		}
		case 3: off += 16; break;
		case 4: off += 6 + (rdi(off + 4) + 1) * 2; break;
		case 5: case 6: off += 6 + rdi(off + 4) * 2; break;
		case 7: case 8: off += 4; break;
		case 9: case 0xc: off += 6; break;
		case 0xa: off += 8; break;
		case 0xb: {
			slot sl;
			for (int i = 0; i < 12; i++) sl.mat[i] = rdf(off + 2 + 2 * i);
			lf.slots.push_back(sl);
			off += 26;
			break;
		}
		default: return;
		}
	}
}

// Instead of drawing the frame: keep its list when it is a new one. The object count stands in
// for the polygon count (the clean scene changes go by it); a frame with nothing in 3D is drawn,
// so a black curtain can be told from a scene being built.
void model1::record_frame()
{
	// No new list this frame (the game draws its 3D every other frame at times, as on the name
	// entry's turning ring): the board keeps showing the last one, so the picture carries on.
	int objects = m_logic[1].list.empty() ? 0 : m_logic[1].objects;
	if ((m_listctl[1] & 0x1f) == 0x1f) {
		const uint16_t *shown = picture_list();
		logic_frame lf;
		lf.list.assign(shown, shown + 0x8000);
		uint64_t h = 1469598103934665603ull;
		for (uint16_t w : lf.list) h = (h ^ w) * 1099511628211ull;
		lf.hash = h;
		lf.frame = int(m_frame);
		lf.hscr = m_tiles.tile_r(0x5002);
		lf.vscr = m_tiles.tile_r(0x5006);
		index_objects(lf);
		objects = lf.objects;
		static const bool log = getenv("VF_LOG_SMOOTH") != nullptr;
		if (lf.hash != m_logic[1].hash) {
			m_logic[0] = std::move(m_logic[1]);
			m_logic[1] = std::move(lf);
		}
	}
	// the sky's scroll, every frame, with the fractions the game dropped: the smooth sky goes by these
	const uint16_t h = m_tiles.tile_r(0x5002), v = m_tiles.tile_r(0x5006);
	sky_sample &sm = m_sky_hist[m_frame % SKY_HIST];
	sm = { int(m_frame), h, v, float(h & 0x1ff) + m_sky_frac_h, float(v & 0x3ff) + m_sky_frac_v };
	// and the next frame's, when the game's routine wrote this one: it runs again at the next vblank
	// from what is in its RAM now (the game moves the camera just after the routine, not before)
	if (m_sky_written == m_frame && m_sky_read_h && m_sky_read_v) {
		const sky_scroll next = sky_scroll_from_ram(!m_sky_between);
		sm.nx = next.hx;
		sm.ny = next.vy;
		sm.next = true;
	}
	m_render_done = 0;            // the list's uploads are made at the next vblank, as when frames are skipped
	m_last_quads = objects;
	if (objects == 0) {
		// nothing in 3D (a curtain, a 2D screen): the next scene starts afresh, not blended from this
		m_logic[0] = m_logic[1] = logic_frame{};
		screen_update(m_bitmap, m_bitmap.cliprect());
		m_unshown = true;
	}
}

// Whether the sky's scroll jumped in the newest frame (more than a pan moves it).
bool model1::sky_settling() const
{
	const int f = int(m_frame) - 1;
	const sky_sample &a = m_sky_hist[f % SKY_HIST], &b = m_sky_hist[(f - 1 + SKY_HIST) % SKY_HIST];
	if (a.frame != f || b.frame != f - 1) return false;
	const int dh = ((a.h - b.h + 256) & 511) - 256, dv = ((a.v - b.v + 512) & 1023) - 512;
	return std::abs(dh) > 8 || std::abs(dv) > 8;
}

void model1::smooth_cut()
{
	m_logic[0] = m_logic[1];
}

// The picture for a moment `now`, in frames (the newest frame's number plus how far into it).
bool model1::render_smooth(double now, bool shown)
{
	m_draw_time = now - 2;     // the picture is a game frame (two video frames) behind
	// what redraw() needs to draw it again: the moment and the frame it's drawn in
	auto drawn = [&] {
		m_unshown = !shown || m_parts;                  // (in parts: frame() holds only the 3D)
		if (shown) { m_shown = true; m_shown_now = now; m_shown_frame = m_frame; }
	};
	if (m_logic[1].list.empty()) { screen_update(m_bitmap, m_bitmap.cliprect()); drawn(); return true; }
	const logic_frame &a = m_logic[0], &b = m_logic[1];
	double alpha = 1;
	if (!a.list.empty() && b.frame > a.frame) {
		const double span = b.frame - a.frame;
		alpha = std::clamp((now - span - a.frame) / span, 0.0, 1.0);
	}
	m_alpha = float(alpha);
	m_replaying = true;
	m_smooth_stats[0] = m_smooth_stats[1] = m_smooth_stats[2] = 0;
	blend_slots();
	// A cut can't be blended: the older picture stays until the newer one's moment (as the
	// blend would), so the cut lands when the game's sky, written two frames later, changes too.
	m_replay_older = m_blend_cut && alpha < 1;
	if (m_blend_cut && b.frame != m_cut_frame) m_cut_frame = b.frame;
	// After a cut the game moves the sky into place over a few frames: until it has stopped
	// jumping (at most 4 frames), the picture before the cut stays.
	// (redraw() draws a picture that was shown: never one held back)
	if (!m_redrawing && !m_replay_older && m_cut_frame >= 0 && now < m_cut_frame + 2 + 4 && sky_settling()) {
		m_replaying = false;
		m_alpha = 1;
		return false;
	}
	const auto render_done = m_render_done;     // emulation state: drawing again mustn't touch it
	const int quads = m_last_quads;
	screen_update(m_bitmap, m_bitmap.cliprect());
	m_render_done = render_done;
	m_last_quads = quads;
	m_replaying = false;
	m_replay_older = false;
	drawn();
	static const bool log = getenv("VF_LOG_SMOOTH") != nullptr;
	if (log && alpha < 1)
		fprintf(stderr, "R %.2f a=%.2f unmatched %d jumped %d blended %d\n", now, alpha, m_smooth_stats[0], m_smooth_stats[1], m_smooth_stats[2]);
	m_alpha = 1;
	return true;
}

// As a frame starts while the picture on screen is the current one: what it was drawn from, in
// case frames run on without a new picture shown (a scene change held, the hold after a cut) and
// it has to be drawn again (redraw, at new settings while paused). Every frame in play: about
// 300 KB copied (the tile graphics only when they changed).
void model1::keep_still()
{
	still_state &s = m_still;
	s.frame = m_frame;
	s.logic[0] = m_logic[0];
	s.logic[1] = m_logic[1];
	s.cut_frame = m_cut_frame;
	std::copy(std::begin(m_sky_hist), std::end(m_sky_hist), s.sky_hist);
	s.tile_ram = m_tiles.tile_ram;
	if (s.char_gen != m_tiles.char_gen || s.char_ram.size() != m_tiles.char_ram.size()) {
		s.char_ram = m_tiles.char_ram;
		s.char_gen = m_tiles.char_gen;
	}
	std::copy(std::begin(m_pens), std::end(m_pens), s.pens.begin());
	s.palette = m_paletteram16_ram;
	s.color_xlat = m_color_xlat_ram;
	std::copy(std::begin(m_view->lightparams), std::end(m_view->lightparams), s.lightparams.begin());
	s.light = m_view->light;
	s.spec_enable = m_view->spec_enable;
	memcpy(s.camera, &m_ram_b[0x17f0], 4);
	s.fight_cam = m_fight_cam;
	s.sky_now = m_sky_now; s.sky_last = m_sky_last;
	s.sky_base = m_sky_base; s.sky_last_tx0 = m_sky_last_tx0;
}

// Exchanges the kept state with the live one (twice: in to draw the kept moment, back out).
void model1::swap_still()
{
	still_state &s = m_still;
	std::swap(m_frame, s.frame);
	std::swap(m_logic[0], s.logic[0]);
	std::swap(m_logic[1], s.logic[1]);
	std::swap(m_cut_frame, s.cut_frame);
	std::swap_ranges(std::begin(m_sky_hist), std::end(m_sky_hist), s.sky_hist);
	m_tiles.swap_rams(s.tile_ram, s.char_ram);
	m_tiles_updated = ~0ull;                         // decoded again from the RAMs now in place
	std::swap_ranges(std::begin(m_pens), std::end(m_pens), s.pens.begin());
	m_paletteram16_ram.swap(s.palette);
	m_color_xlat_ram.swap(s.color_xlat);
	m_paletteram16 = m_paletteram16_ram.data();
	m_color_xlat = m_color_xlat_ram.data();
	std::swap_ranges(std::begin(m_view->lightparams), std::end(m_view->lightparams), s.lightparams.begin());
	std::swap(m_view->light, s.light);
	std::swap(m_view->spec_enable, s.spec_enable);
	std::swap_ranges(&m_ram_b[0x17f0], &m_ram_b[0x17f4], s.camera);   // (the camera pointer: work RAM, put back after)
	std::swap(m_fight_cam, s.fight_cam);
	std::swap(m_sky_now, s.sky_now); std::swap(m_sky_last, s.sky_last);
	std::swap(m_sky_base, s.sky_base); std::swap(m_sky_last_tx0, s.sky_last_tx0);
}

// The blended matrix of every slot in the newer list. Slots pair by position when the two lists
// have the same shape (most of a fight), else by the first object drawn in them. A picture where
// many slots jump is a camera cut: nothing is blended, so parts never come apart.
void model1::blend_slots()
{
	const logic_frame &a = m_logic[0], &b = m_logic[1];
	m_blend.assign(b.slots.size(), {});
	m_blend_ok.assign(b.slots.size(), 0);
	m_blend_cut = false;
	if (m_alpha >= 1 || a.slots.empty()) return;
	// pair the slots in order, like a diff: the same first model pairs; a different model in the
	// same place (a hand, a face swapped) pairs too; slots that appeared or went are skipped
	std::vector<int> pair(b.slots.size(), -1);
	auto first = [](const slot &sl) { return sl.polys.empty() ? 0u : sl.polys[0]; };
	size_t i = 0, j = 0;
	const size_t na = a.slots.size(), nb = b.slots.size();
	while (i < na && j < nb) {
		if (first(a.slots[i]) == first(b.slots[j])) { pair[j++] = int(i++); continue; }
		size_t ka = 1, kb = 1;
		while (i + ka < na && ka <= 8 && first(a.slots[i + ka]) != first(b.slots[j])) ka++;
		while (j + kb < nb && kb <= 8 && first(b.slots[j + kb]) != first(a.slots[i])) kb++;
		const bool a_skip = i + ka < na && ka <= 8, b_skip = j + kb < nb && kb <= 8;
		if (a_skip && (!b_skip || ka <= kb)) i += ka;       // slots in the older list that went
		else if (b_skip) j += kb;                            // slots that are new
		else pair[j++] = int(i++);                           // the same place, a different model
	}
	int jumps = 0, paired = 0, missing = 0;
	for (size_t i = 0; i < b.slots.size(); i++) {
		if (pair[i] < 0 && !b.slots[i].polys.empty()) missing++;
		if (pair[i] < 0 || b.slots[i].polys.empty()) continue;
		const float *pa = a.slots[pair[i]].mat, *pb = b.slots[i].mat;
		const float dx = pb[9] - pa[9], dy = pb[10] - pa[10], dz = pb[11] - pa[11];
		const float depth = std::max({ std::fabs(pa[11]), std::fabs(pb[11]), 1.0f });
		paired++;
		// a part that moved far, or turned sharply (a cut can keep the parts' places and swing the
		// camera round them: blending two far-apart turns squashes and stretches the shapes)
		bool turned = false;
		for (int r = 0; r < 9 && !turned; r += 3) {
			const float la = pa[r] * pa[r] + pa[r + 1] * pa[r + 1] + pa[r + 2] * pa[r + 2];
			const float lb = pb[r] * pb[r] + pb[r + 1] * pb[r + 1] + pb[r + 2] * pb[r + 2];
			const float dot = pa[r] * pb[r] + pa[r + 1] * pb[r + 1] + pa[r + 2] * pb[r + 2];
			if (la > 0 && lb > 0 && dot < 0.77f * std::sqrt(la * lb)) turned = true;     // over ~40 degrees
		}
		if (dx * dx + dy * dy + dz * dz > 0.09f * depth * depth || turned) jumps++;    // fast, or a cut
		for (int k = 0; k < 12; k++) m_blend[i][k] = pa[k] + (pb[k] - pa[k]) * m_alpha;
		m_blend_ok[i] = 1;
	}
	// A camera cut moves nearly every part at once (fast moves only a few): then, or when a part
	// can't be paired (it would come apart from the blended ones), the newer frame is shown whole.
	m_blend_cut = jumps * 2 > paired || missing > 0;
	if (m_blend_cut) std::fill(m_blend_ok.begin(), m_blend_ok.end(), 0);
	m_smooth_stats[1] = jumps;
	static const bool logm = getenv("VF_LOG_SMOOTH") != nullptr;
	if (logm && (jumps || missing)) fprintf(stderr, "M jumps %d missing %d paired %d slots %zu/%zu\n", jumps, missing, paired, na, nb);
	for (size_t i = 0; i < b.slots.size(); i++)
		if (!b.slots[i].polys.empty()) m_smooth_stats[m_blend_ok[i] ? 2 : 0]++;
}

// While replaying: the matrix for the object being drawn (the current slot's, blended).
bool model1::smooth_matrix(uint32_t, float *out)
{
	if (m_replay_older || m_replay_slot < 0 || m_replay_slot >= int(m_blend_ok.size()) || !m_blend_ok[m_replay_slot]) return false;
	memcpy(out, m_blend[m_replay_slot].data(), 12 * sizeof(float));
	return true;
}

// The game's sky scroll (the routine at ff49a1, at the start of vblank) comes from the camera:
// across, its heading (508004: 1/32 pixel, 2048 pixels for a turn); up and down, 508006 (whole
// pixels). Each is mixed with the last game frame's (508008, 50800a): halfway across on the frames
// between the game's 30 a second, and a quarter or three quarters up and down; then written to
// tile map 2's scroll in whole pixels (h: the heading >> 5; v: a sum >> 1). When a write is that
// sum, the fraction it dropped is kept for smooth motion's sky (worked out again from the RAM it
// read: nothing writes those in between).
model1::sky_scroll model1::sky_scroll_from_ram(bool between) const
{
	auto rd = [&](uint32_t x) { return uint16_t(m_ram_b[x - 0x500000] | m_ram_b[x - 0x500000 + 1] << 8); };
	uint16_t r1 = rd(0x508004);
	if (between) r1 = uint16_t(r1 - (int16_t(uint16_t(r1 - rd(0x508008))) >> 1));
	uint16_t r3 = rd(0x508006), r4 = rd(0x50800a);
	const uint16_t d = uint16_t(r3 - r4);
	if (between) r3 = uint16_t(r3 - (int16_t(d) >> 1));
	else r4 = uint16_t(r4 + (int16_t(d) >> 1) + (d & 1));   // (rounded: the bit shifted out added back)
	const uint16_t sum = uint16_t(r3 + r4);
	sky_scroll s;
	s.h = (r1 >> 5) & 0x3ff;
	s.v = uint16_t((((sum >> 1) + 0x21) & 0x3ff) | (r3 & 0xe000));
	s.hx = float(s.h & 0x1ff) + (r1 & 31) / 32.0f;
	s.vy = float(s.v & 0x3ff) + (sum & 1) * 0.5f;
	return s;
}

void model1::sky_scroll_written(uint32_t a)
{
	m_sky_between = m_ram_b[0x501] & 1;
	const sky_scroll s = sky_scroll_from_ram(m_sky_between);
	if (a == 0x70a004) {
		m_sky_read_h = s.h == m_tiles.tile_r(0x5002);         // (anything else written: no fraction)
		m_sky_frac_h = m_sky_read_h ? s.hx - float(s.h & 0x1ff) : 0;
	} else {
		m_sky_read_v = s.v == m_tiles.tile_r(0x5006);
		m_sky_frac_v = m_sky_read_v ? s.vy - float(s.v & 0x3ff) : 0;
	}
	m_sky_written = m_frame;
}

// While replaying: how far to move the sky layer (arcade pixels) from where the tile chip has it
// now to where it was at the moment drawn: the game's own scroll with its fractions, each frame's
// (straight between them, as the 3D's blend; past the newest frame, toward the next one's, worked
// out as the newest ended). At a frame's own moment it is that frame's scroll, whose whole pixels
// are what the game drew. The sky's map x is -hscroll, its y +vscroll (1024 rows, in two maps).
void model1::smooth_sky_shift(float &dx, float &dy) const
{
	dx = dy = 0;
	if (!m_replaying) return;
	// The game writes the sky's scroll two frames after the display list it goes with (at a camera
	// cut the 3D changes at frame C and the sky at C+2): the sky drawn with the list for moment
	// m is the scroll at m+2, which smooth motion, two frames behind the game, has up to the
	// newest frame and the next.
	constexpr double SKY_LAG = 2;
	const double t = m_draw_time + SKY_LAG;
	const int newest = int(m_frame) - 1;
	if (newest < 0) return;
	const sky_sample &cur = m_sky_hist[newest % SKY_HIST];
	if (cur.frame != newest) return;
	// frame f's scroll less the newest's (unwrapped), or false when it's not kept
	auto at = [&](int f, bool horizontal, double &out) {
		if (f < 0 || f > newest || f <= newest - SKY_HIST) return false;
		const sky_sample &s = m_sky_hist[f % SKY_HIST];
		if (s.frame != f) return false;
		const double w = horizontal ? 512 : 1024, d = horizontal ? s.hx - cur.hx : s.vy - cur.vy;
		out = d - w * std::round(d / w);
		return true;
	};
	auto moment = [&](bool horizontal) {
		const int k = std::min(int(std::floor(t)), newest);
		double fr = t - k;
		double a, b;
		if (!at(k, horizontal, a)) return 0.0;
		if (fr <= 0) return a;
		// a jump (a cut) isn't crossed: the scroll changes at the frame, as the game's
		if (k < newest) return at(k + 1, horizontal, b) && std::fabs(b - a) <= 24 ? a + (b - a) * fr : a;
		fr = std::min(fr, 1.0);
		if (cur.next) {                                   // a == 0: the newest frame's
			const double w = horizontal ? 512 : 1024, d = horizontal ? cur.nx - cur.hx : cur.ny - cur.vy;
			b = d - w * std::round(d / w);
			return std::fabs(b) <= 24 ? b * fr : 0.0;
		}
		return at(k - 1, horizontal, b) && std::fabs(a - b) <= 24 ? a + (a - b) * fr : a;   // else at its speed
	};
	const int h_now = m_tiles.tile_r(0x5002) & 0x1ff, v_now = m_tiles.tile_r(0x5006) & 0x3ff;
	const double h_t = cur.hx + moment(true), v_t = cur.vy + moment(false);
	auto wrapped = [](double d, double w) { return d - w * std::round(d / w); };
	dx = float(wrapped(h_now - h_t, 512));               // sample at x + (h_now - h_t)
	dy = float(wrapped(v_t - v_now, 1024));              // and y + (v_t - v_now)
	if (std::fabs(dx) > 24 || std::fabs(dy) > 24) dx = dy = 0;
}
