// Headless runner: boots VF on the interpreter and writes frames as PNGs.
//   vfrun <regions dir> <frames> [every N] [outdir]
#include "../src/host/regions.h"
#include "../src/host/png.h"
#include <tuple>
#include "../src/host/backgrounds.h"
#include "../src/host/scene_sync.h"
#include "../src/rt/rec_rt.h"
#include <chrono>
#include <cmath>
#include <csignal>

static uint32_t g_ring[256];
static unsigned g_ring_pos;
static model1 *g_m;

static void dump_ring(int)
{
	fprintf(stderr, "last instructions:\n");
	for (unsigned i = 0; i < 256; i++)
		fprintf(stderr, "%06x%s", g_ring[(g_ring_pos + i) & 255], (i & 15) == 15 ? "\n" : " ");
	if (g_m) {
		fprintf(stderr, "regs:");
		for (int r = 0; r < 32; r++) fprintf(stderr, " R%d=%08x", r, g_m->cpu().reg(r));
		fprintf(stderr, "\n");
	}
	_exit(1);
}

int main(int argc, char **argv)
{
	std::string dir = argc > 1 ? argv[1] : "build/vf";
	int frames = argc > 2 ? atoi(argv[2]) : 600;
	int every = argc > 3 ? atoi(argv[3]) : 60;
	std::string out = argc > 4 ? argv[4] : "build/frames";
	model1 m(load_regions(dir));
	if (const char *sc = getenv("VF_SCALE")) m.set_render_scale(atoi(sc));
	if (const char *as = getenv("VF_ASPECT")) m.set_aspect(strtof(as, nullptr));
	const char *wav = getenv("VF_WAV");
	if (wav) m.enable_sound();
	if (wav) m.set_volumes(getenv("VF_MUSIC") ? strtof(getenv("VF_MUSIC"), nullptr) : 1,    // stems: 0 mutes one
						   getenv("VF_EFFECTS") ? strtof(getenv("VF_EFFECTS"), nullptr) : 1);
	if (const char *bg = getenv("VF_BACKGROUNDS")) load_backgrounds(m, bg);   // hi-res skies
	if (getenv("VF_PCMODE")) {    // the saved settings (free play) and no FREE PLAY notices, as the game
		for (auto [file, dst, n] : { std::tuple{ "/nvram.sav", m.nvram().data(), m.nvram().size() },
									 std::tuple{ "/eeprom_93c46.sav", m.eeprom(), size_t(model1::EEPROM_SIZE) } })
			if (FILE *f = fopen((dir + file).c_str(), "rb")) { fread(dst, 1, n, f); fclose(f); }
		m.hide_free_play_text();
	}
	m.reset();
	g_m = &m;
	if (getenv("VF_REC")) {
		rec::attach(m.cpu());
		m.run_cpu = [](int n) { rec::run(n); };
	}
	std::vector<int16_t> audio;
	if (getenv("VF_TRACE")) {
		signal(SIGABRT, dump_ring);
		m.on_instruction = [](uint32_t pc) { g_ring[g_ring_pos++ & 255] = pc; };
	}
	auto t0 = std::chrono::steady_clock::now();
	std::vector<std::pair<int, int>> clips;
	if (const char *c = getenv("VF_CLIPS"))
		for (const char *q = c; *q;) {
			int a = 0, b = 0, used = 0;
			if (sscanf(q, "%d-%d%n", &a, &b, &used) < 2) break;
			clips.push_back({a, b});
			q += used;
			if (*q == ',') q++;
		}
	auto in_clip = [&](int f) {
		if (clips.empty()) return true;
		for (auto &[a, b] : clips) if (f >= a && f <= b) return true;
		return false;
	};
	FILE *raw = getenv("VF_RAW") ? fopen(getenv("VF_RAW"), "wb") : nullptr;
	scene_sync sync;                                // VF_SCENE_SYNC=1: clean scene changes, as the game
	sync.enabled = getenv("VF_SCENE_SYNC") != nullptr;
	// VF_SMOOTH=n: smooth motion, n pictures per frame (n x 57.5 fps out); PNGs get a suffix
	const int smooth = getenv("VF_SMOOTH") ? std::max(1, atoi(getenv("VF_SMOOTH"))) : 0;
	if (smooth) m.set_smooth(true);
	if (getenv("VF_BG_FILTER")) m.set_sky_filter(atoi(getenv("VF_BG_FILTER")) != 0);
	std::vector<uint32_t> shown(size_t(m.frame_w()) * m.frame_h(), 0);   // the picture on screen
	sync.start();
	uint32_t rng = getenv("VF_SEED") ? uint32_t(atoi(getenv("VF_SEED"))) : 12345;
	const bool script = getenv("VF_SCRIPT") != nullptr;
	for (int f = 1; f <= frames; f++) {
		if (script) {
			// as vfcov: attract, coins, a two-player game, random moves
			uint8_t in0 = 0xff, in1 = 0xff, in2 = 0xff;
			int phase = f % 6000;
			if (phase >= 3000 && phase < 3006) in0 &= ~0x01;
			if (phase >= 3060 && phase < 3066) in0 &= ~0x01;
			if (phase >= 3120 && phase < 3126) in0 &= ~0x10;
			if (phase >= 3200 && phase < 3206) in0 &= ~0x20;
			if (phase >= 3130 && (f % 7) == 0) {
				rng = rng * 1103515245 + 12345;
				in1 = uint8_t(~((rng >> 16) & 0xf7));
				in2 = uint8_t(~((rng >> 8) & 0xf7));
			}
			m.m_inputs[0] = in0; m.m_inputs[1] = in1; m.m_inputs[2] = in2;
		}
		m.run_frame();
		const bool keep = in_clip(f);
		const size_t npx = size_t(m.frame_w()) * m.frame_h();
		const bool was_holding = sync.holding();
		const bool showing = sync.show(m);
		if (was_holding && !sync.holding()) m.smooth_cut();     // as the game: a new scene isn't blended from the old
		if (getenv("VF_LOG_FRAMES") && keep) {     // frame, polygons, camera, a 1/64 sample checksum of the picture
			uint64_t h = 0;
			const auto &b = m.frame();
			for (size_t i = 0; i < size_t(m.frame_w()) * m.frame_h(); i += 61) h = h * 31 + b.data()[i];
			fprintf(stderr, "F %d %d %06x %016llx\n", f, m.frame_quads(), m.camera(), (unsigned long long)h);
		}
		if (wav) { if (keep) audio.insert(audio.end(), m.m_audio_out.begin(), m.m_audio_out.end()); m.m_audio_out.clear(); }
		for (int k = 0; k < std::max(1, smooth); k++) {
			if (smooth && (showing || !sync.enabled)) m.render_smooth(double(m.frame_number() - 1) + double(k) / smooth);
			if (showing || !sync.enabled || shown.size() != npx) shown.assign(m.frame().data(), m.frame().data() + npx);
			const uint32_t *pic = sync.enabled ? shown.data() : m.frame().data();
			if (raw && keep) fwrite(pic, 4, npx, raw);
			if (every && f % every == 0 && keep) {
				char path[512];
				if (smooth > 1) snprintf(path, sizeof path, "%s/f%05d_%d.png", out.c_str(), f, k);
				else snprintf(path, sizeof path, "%s/f%05d.png", out.c_str(), f);
				write_png(path, pic, m.frame_w(), m.frame_h());
				printf("frame %d pc=%06x\n", f, m.cpu().pc());
			}
		}
	}
	if (raw) fclose(raw);
	if (wav) {
		FILE *w = fopen(wav, "wb");
		uint32_t n = uint32_t(audio.size() * 2), rate = m1audio::OUT_RATE;
		auto u32 = [&](uint32_t v) { fwrite(&v, 4, 1, w); };
		auto u16 = [&](uint16_t v) { fwrite(&v, 2, 1, w); };
		fwrite("RIFF", 1, 4, w); u32(36 + n); fwrite("WAVEfmt ", 1, 8, w); u32(16); u16(1); u16(2);
		u32(rate); u32(rate * 4); u16(4); u16(16); fwrite("data", 1, 4, w); u32(n);
		fwrite(audio.data(), 2, audio.size(), w);
		fclose(w);
		double sum = 0; int peak = 0;
		for (int16_t v : audio) { sum += double(v) * v; peak = std::max(peak, abs(int(v))); }
		printf("audio: %zu samples, rms %.0f, peak %d\n", audio.size() / 2, sqrt(sum / std::max<size_t>(1, audio.size())), peak);
	}
	double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
	fprintf(stderr, "%d frames in %.2fs (%.0f fps)\n", frames, s, frames / s);
}
