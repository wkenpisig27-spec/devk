#pragma once

#include <cstdint>

// M3 phase 1: opaque session identity (slot + generation), wire-compatible with 8-byte payloads.
struct SessionHandle {
	static constexpr uint32_t kInvalidSlot = 0xFFFFFFFFu;

	uint32_t slot{kInvalidSlot};
	uint32_t generation{0};

	bool IsValid() const { return slot != kInvalidSlot; }

	bool operator==(const SessionHandle& other) const {
		return slot == other.slot && generation == other.generation;
	}

	bool operator!=(const SessionHandle& other) const { return !(*this == other); }
};
