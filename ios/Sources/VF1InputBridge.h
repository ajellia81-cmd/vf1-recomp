#pragma once

#include <array>
#include "../../src/host/controls.h"

// UI-neutral state consumed once per emulation frame by the SDL host integration.
// Values use the existing control_action enum so saved desktop mappings remain authoritative.
class VF1InputSnapshot {
public:
	void set(control_action action, bool down) { m_down[size_t(action)] = down; }
	bool down(control_action action) const { return m_down[size_t(action)]; }
	void clear() { m_down.fill(false); }
private:
	std::array<bool, C_COUNT> m_down{};
};
