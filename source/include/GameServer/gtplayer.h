/********************************************************************
	created:	2005/02/28
	created:	28:2:2005   9:23
	filename: 	d:\project\Server\GameServer\src\gtplayer.h
	file path:	d:\project\Server\GameServer\src
	file base:	gtplayer
	file ext:	h
	author:		claude

	purpose:
*********************************************************************/

#ifndef GTPLAYER_H_
#define GTPLAYER_H_

#include <cstdint>  // For uintptr_t

class GateServer;
// Unique Player structure
struct uplayer {
	uplayer() : m_dwDBChaId(0), pGate(nullptr), m_ulGateAddr(0) {}
	uplayer(char const* gt_name, uintptr_t gt_addr, DWORD cha_id) { Init(gt_name, gt_addr, cha_id); }

	void Init(char const* gt_name, uintptr_t gt_addr, DWORD cha_id);

	uplayer& operator=(uplayer const& up) {
		this->m_dwDBChaId = up.m_dwDBChaId;
		this->pGate = up.pGate;
		this->m_ulGateAddr = up.m_ulGateAddr;
		return *this;
	}

	DWORD m_dwDBChaId; // 唯一ID

	// 确定这个Player
	GateServer* pGate;
	uintptr_t m_ulGateAddr; // 在 GateServer 进程中的虚拟地址 (pointer-sized for platform compatibility)
};

struct GatePlayer {
protected:
	GatePlayer() : ply(), Next(nullptr), Prev(nullptr), m_tmplist(nullptr), m_generation(0) {}
	~GatePlayer() {
		Next = nullptr;
		Prev = nullptr;
	}

public:
	void SetGate(GateServer* gt) { ply.pGate = gt; }
	GateServer* GetGate() const { return ply.pGate; }

	void SetGateAddr(uintptr_t gt_addr) { ply.m_ulGateAddr = gt_addr; }
	uintptr_t GetGateAddr() const { return ply.m_ulGateAddr; }

	void SetDBChaId(DWORD dwDBChaId) { ply.m_dwDBChaId = dwDBChaId; }
	DWORD GetDBChaId(void) { return ply.m_dwDBChaId; }

	GatePlayer*& GetNextPlayer() { return m_tmplist; }

	virtual void OnLogin() {}
	virtual void OnLogoff() {}
	
	// Generation counter for pointer validation
	uint32_t GetGeneration() const { return m_generation; }
	void SetGeneration(uint32_t gen) { m_generation = gen; }

	GatePlayer* Next;
	GatePlayer* Prev;

private:
	uplayer ply;
	uint32_t m_generation;  // Session generation for pointer validation

	GatePlayer* m_tmplist; // 特定发包接口之用，名称以后可能会调整
};

#endif