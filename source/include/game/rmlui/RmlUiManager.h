#pragma once

#include <windows.h>

// Thin facade that owns RmlUi interfaces/context and plugs into the game loop.
// Runs as a parallel overlay on top of the existing CFormMgr UI.
class CRmlUiManager {
public:
	static CRmlUiManager& Instance();

	bool Init(HWND hwnd);
	void Shutdown();
	bool IsReady() const { return m_ready; }

	void Update();
	void Render();

	// DirectInput mouse path (same flags as MPGameApp: M_LDown, M_Move, ...)
	void ProcessMouse(int x, int y, DWORD mouseKey);
	void ProcessMouseWheel(int nScroll);

	// Win32 keyboard path. Returns true if RmlUi consumed the event.
	bool ProcessKeyDown(int win32Key);
	bool ProcessKeyUp(int win32Key);
	bool ProcessTextInput(char character);

	bool IsMouseInteracting() const;

	void SetEnabled(bool enabled) { m_enabled = enabled; }
	bool IsEnabled() const { return m_enabled; }

	// Hide account/region/server overlays (they persist across scene switches).
	void HideLoginForms();
	void HideSelectChaForm();
	void HideInventoryForm();
	void HideBankForm();
	void HideCharacterForm();

private:
	CRmlUiManager() = default;
	~CRmlUiManager() = default;
	CRmlUiManager(const CRmlUiManager&) = delete;
	CRmlUiManager& operator=(const CRmlUiManager&) = delete;

	void SyncViewport();

	bool m_ready = false;
	bool m_enabled = true;
	bool m_leftDown = false;
	bool m_rightDown = false;
	bool m_middleDown = false;
	int m_lastX = -1;
	int m_lastY = -1;
	HWND m_hwnd = nullptr;
};
