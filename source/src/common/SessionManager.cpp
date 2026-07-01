#include "common/SessionManager.h"

#include "util/log.h"

#include <algorithm>

SessionHandle SessionManager::Allocate(void* owner) {
	if (!owner) {
		return {};
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	uint32_t slot = kMaxSlots;
	if (!m_freeList.empty()) {
		slot = m_freeList.back();
		m_freeList.pop_back();
	} else if (m_nextSlot < kMaxSlots) {
		slot = m_nextSlot++;
	} else {
		const uint32_t activeEstimate =
			m_nextSlot - static_cast<uint32_t>(m_freeList.size());
		LG("SessionManager",
		   "Allocate REJECT: slot table exhausted active~=%u cap=%u free=%zu owner=%p\n",
		   activeEstimate, kMaxSlots, m_freeList.size(), owner);
		return {};
	}

	SlotEntry& entry = m_slots[slot];
	++entry.generation;
	if (entry.generation == 0) {
		entry.generation = 1;
	}
	entry.owner = owner;
	return SessionHandle{slot, entry.generation};
}

void SessionManager::Bind(SessionHandle handle, void* owner) {
	if (!handle.IsValid() || handle.slot >= kMaxSlots || !owner) {
		return;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	SlotEntry& entry = m_slots[handle.slot];
	entry.generation = handle.generation;
	entry.owner = owner;

	m_freeList.erase(
		std::remove(m_freeList.begin(), m_freeList.end(), handle.slot),
		m_freeList.end());
	if (handle.slot >= m_nextSlot) {
		m_nextSlot = handle.slot + 1;
	}
}

void SessionManager::Release(SessionHandle handle) {
	if (!handle.IsValid() || handle.slot >= kMaxSlots) {
		return;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	SlotEntry& entry = m_slots[handle.slot];
	if (entry.generation != handle.generation) {
		return;
	}
	entry.owner = nullptr;
	m_freeList.push_back(handle.slot);
}

void* SessionManager::Resolve(SessionHandle handle) const {
	if (!handle.IsValid() || handle.slot >= kMaxSlots) {
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	const SlotEntry& entry = m_slots[handle.slot];
	if (entry.generation != handle.generation || entry.owner == nullptr) {
		return nullptr;
	}
	return entry.owner;
}

bool SessionManager::Validate(SessionHandle handle, void* owner) const {
	return Resolve(handle) == owner;
}
