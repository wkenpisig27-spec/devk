#include "stdafx.h"
#include "steadyframe.h"
#ifdef PKO_PLATFORM_WINDOWS
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

DWORD CSteadyFrame::_dwFPS = 0;
DWORD CSteadyFrame::_dwTargetFPS = 30;
bool  CSteadyFrame::_bFramerate  = false;
DWORD CSteadyFrame::_dwTimeSpace = 0;

bool CSteadyFrame::Init()
{
	_dwRefreshFPS = _bFramerate ? 60 : 30;
	if (_dwTargetFPS > 0)
		_dwRefreshFPS = _dwTargetFPS;
	SetFPS(_dwRefreshFPS);

	_dwTotalTime = 0;
	_dwRunCount  = 0;
	_bStop.store(false, std::memory_order_relaxed);

#ifdef PKO_PLATFORM_WINDOWS
	timeBeginPeriod(1);  // Raise Windows scheduler resolution to ~1 ms
#endif

	_hThread = std::thread([this] { _Sleep(); });
	LG("threadid", "SteadyFrame thread started (target %u fps)\n", _dwTargetFPS);
	return true;
}

void CSteadyFrame::Exit()
{
	_bStop.store(true, std::memory_order_release);
	if (_hThread.joinable())
		_hThread.join();

#ifdef PKO_PLATFORM_WINDOWS
	timeEndPeriod(1);
#endif
	LG("threadid", "_SleepThreadProc end\n");
}

void CSteadyFrame::_Sleep()
{
	int   nCount  = 0;
	DWORD dwTime  = GetTickCount();
	float fRate   = 0.0f;
	DWORD dwLastTime = GetTickCount();
	DWORD s_dwSlower = 0;

	while (!_bStop.load(std::memory_order_relaxed))
	{
		DWORD dwNow     = GetTickCount();
		DWORD dwElapsed = dwNow - dwLastTime;
		dwLastTime = dwNow;

		DWORD dwSleep = _dwTimeSpace > s_dwSlower ? _dwTimeSpace - s_dwSlower : 0;

		// Compensate if last iteration took longer than the frame budget
		if (dwElapsed < dwSleep * 2) {
			if (dwElapsed > dwSleep) {
				s_dwSlower = dwElapsed - dwSleep;
				dwSleep   -= s_dwSlower;
			} else {
				s_dwSlower = 0;
			}

			if (dwSleep > 100)
				dwSleep = 100;

			if (dwSleep)
				Sleep(dwSleep);
		} else {
			s_dwSlower = dwSleep;
		}

		LG("_Sleep", "%d %d\n", dwTime, nCount);

		_lRun.fetch_add(1, std::memory_order_release);

		nCount++;
		if (nCount >= 60)
		{
			DWORD dwCurTime = GetTickCount();
			DWORD dwWindow  = dwCurTime - dwTime;
			if (dwWindow > 0)
			{
				fRate = (float)_dwTotalTime / (float)dwWindow;
				if      (fRate < 0.5f)  _dwRefreshFPS = _dwFPS + 3;
				else if (fRate < 0.6f)  _dwRefreshFPS = _dwFPS + 2;
				else if (fRate < 0.7f)  _dwRefreshFPS = _dwFPS + 1;
				else if (fRate > 0.98f) _dwRefreshFPS = _dwFPS >= 3 ? _dwFPS - 3 : 8;
				else if (fRate > 0.95f) _dwRefreshFPS = _dwFPS >= 2 ? _dwFPS - 2 : 8;
				else if (fRate > 0.9f)  _dwRefreshFPS = _dwFPS >= 1 ? _dwFPS - 1 : 8;
				else                    _dwRefreshFPS = _dwFPS;

				// Clamp to [8, _dwTargetFPS]
				if (_dwRefreshFPS > _dwTargetFPS)
					_dwRefreshFPS = _dwTargetFPS;
				if (_dwRefreshFPS < 8)
					_dwRefreshFPS = 8;
			}

			nCount       = 0;
			dwTime       = dwCurTime;
			_dwTotalTime = 0;
			_dwRunCount  = 0;
		}
	}
}