#pragma once
#include <atomic>
#include <thread>

// Frame rate limiter class - supports adaptive FPS up to any target.
// IMPORTANT: constructor must NOT read g_stUISystem (SIOF risk).
// SetFramerate60()/SetTargetFPS() must be called before Init().
class CSteadyFrame
{
public:
	CSteadyFrame() = default;  // SIOF fix: no ini reads in constructor

	static bool GetIsFramerate60()     { return _dwTargetFPS >= 60; }
	static void SetFramerate60(bool v) { SetTargetFPS(v ? 60 : 30); }
	static void SetTargetFPS(DWORD fps){ _dwTargetFPS = fps; _bFramerate = (fps >= 60); SetFPS(fps); }

	bool	Init();
	void	Exit();

	static DWORD GetFPS() { return _dwFPS; }
	static void SetFPS(DWORD v) {
		_dwFPS = v;
		_dwTimeSpace = (int)(1000.0f / (float)_dwFPS);
	}

	// Returns current_fps / 30.0 -- multiply animation velocities by 1.0f/GetAnimMultiplier()
	// to keep them at real-time speed regardless of FPS setting.
	static float GetAnimMultiplier() {
		DWORD fps = _dwFPS;
		return fps > 0 ? (float)fps / 30.0f : 1.0f;
	}

	bool Run() {
		long ticks = _lRun.exchange(0, std::memory_order_acquire);
		if (ticks > 0) {
			RefreshFPS();
			_dwCurTime = GetTickCount();
			_dwRunCount++;
			return true;
		}
		return false;
	}

	DWORD	GetTick()	{ return _dwCurTime; }
	void	End()		{ _dwTotalTime += GetTickCount() - _dwCurTime; }
	void	RefreshFPS()	{ if (_dwFPS != _dwRefreshFPS) SetFPS(_dwRefreshFPS); }

private:
	void _Sleep();

private:
	static DWORD	_dwFPS;
	static DWORD	_dwTargetFPS;
	static bool		_bFramerate;
	static DWORD	_dwTimeSpace;

	std::atomic<long> _lRun{0};
	std::atomic<bool> _bStop{false};
	std::thread       _hThread;

	DWORD	_dwCurTime{0};
	DWORD	_dwTotalTime{0};
	DWORD	_dwRunCount{0};
	DWORD	_dwRefreshFPS{30};
};