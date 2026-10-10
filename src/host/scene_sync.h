// Clean scene changes. When VF changes scene, its pieces arrive on different frames: the 3D
// goes first (a few frames of sky alone), the new 2D layers are written over a frame or two, and
// the new scene's 3D builds up (the floor, then the fighters a game frame later, with the sky
// jumping into place). This holds the last complete picture until the next one is whole:
//   - a frame with no polygons that isn't black is a scene being taken down or built: hold;
//   - black frames are the game's own curtains: shown;
//   - the hold ends when the new scene's polygon count has settled (VF's logic runs at 30 Hz,
//     so that is compared with two frames before), or after 45 frames whatever happens;
//   - a cut between stages (the ending's replays) brings the new stage's sky a few frames before
//     its 3D, with the old stage's 3D still up in between: a sky that turns into another stage's
//     starts a hold too, and while the sky has changed but the polygons are still the old scene's
//     (within an eighth of its count), the hold goes on, for up to 10 frames (the gap is 5-7).
// It only decides which picture to show: the game runs on untouched. After a boot, start()
// holds from black, so the first scene appears whole too.
#pragma once
#include "../machine/model1.h"
#include <cstdlib>

class scene_sync
{
public:
	bool enabled = true;

	void reset() { m_holding = false; m_held = 0; m_q1 = m_q2 = 0; m_sky = -2; m_sky_before = -2; m_q_before = m_q_shown = 0; }
	// A scene is starting from nothing (after a boot or reset): hold until it is whole.
	void start() { reset(); m_holding = true; m_held = 1; }

	// After a frame has run: true to show it, false to keep showing the last one shown.
	bool show(model1 &m)
	{
		const int q = m.frame_quads(), q2 = m_q2;
		m_q2 = m_q1;
		m_q1 = q;
		if (!enabled) { m_holding = false; return true; }
		// the stage sky: a sky map that isn't one (black, a screen between) counts as no sky (-1),
		// and that change too is a cut when the 3D stays up
		const int sky = m.sky_identity(), sky_was = m_sky;
		m_sky = sky;
		const bool blank = is_blank(m);
		auto start_hold = [&] { m_holding = true; m_held = 1; m_sky_before = sky_was; m_q_before = m_q_shown; };
		if (!m_holding) {
			// a scene taken down, or (the ending's replays) a stage cut that keeps the old 3D up
			const bool cut = q > 0 && sky_was != -2 && sky != sky_was;
			if ((q == 0 && !blank) || cut) { start_hold(); return false; }
			m_q_shown = q;
			return true;
		}
		if (blank) return true;                       // a curtain: show it, still waiting
		const bool settled = q > 0 && q2 > 0 && std::abs(q - q2) * 4 <= q;
		// a new stage's sky over the old stage's 3D (it comes a few frames before the new 3D): wait on
		const bool stale = (sky < 0 || sky != m_sky_before) && m_q_before > 0 && std::abs(q - m_q_before) * 8 <= m_q_before;
		// (the new 3D can have as many polygons as the old: after 10 frames a settled picture is taken anyway)
		if ((settled && (!stale || m_held >= 10)) || ++m_held > 45) { m_holding = false; m_q_shown = q; return true; }
		if (settled) ++m_held;
		return false;
	}

	bool holding() const { return m_holding; }

private:
	bool m_holding = false;
	int m_held = 0, m_q1 = 0, m_q2 = 0;
	int m_sky = -2, m_sky_before = -2;   // the stage sky last frame (-2: not seen yet), and before the hold
	int m_q_shown = 0, m_q_before = 0;   // the polygons in the last frame shown, and in the one before the hold

	static bool is_blank(const model1 &m)
	{
		const uint32_t *p = m.frame().data();
		const size_t n = size_t(m.frame_w()) * m.frame_h();
		for (size_t i = 0; i < n; i += 97) {
			const uint32_t c = p[i];
			if (((c >> 16) & 0xff) > 12 || ((c >> 8) & 0xff) > 12 || (c & 0xff) > 12) return false;
		}
		return true;
	}
};
