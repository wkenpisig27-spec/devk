#pragma once

#include "common/SessionHandle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

// Fixed-slot session table with generation stamps (M3 phase 1).
class SessionManager {
public:
	static constexpr uint32_t kMaxSlots = 65536u;

	SessionHandle Allocate(void* owner);
	void Bind(SessionHandle handle, void* owner);
	void Release(SessionHandle handle);
	void* Resolve(SessionHandle handle) const;
	bool Validate(SessionHandle handle, void* owner) const;

private:
	struct SlotEntry {
		void* owner{nullptr};
		uint32_t generation{0};
	};

	mutable std::mutex m_mutex;
	std::array<SlotEntry, kMaxSlots> m_slots{};
	std::vector<uint32_t> m_freeList;
	uint32_t m_nextSlot{0};
};
