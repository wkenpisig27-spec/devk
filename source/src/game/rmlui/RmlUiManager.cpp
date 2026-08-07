#include "rmlui/RmlUi_Win32_Undef.h"
#include "rmlui/RmlUiManager.h"
#include "rmlui/RmlUi_Renderer_DX9.h"
#include "rmlui/RmlUiAccountForm.h"

#include "MPRender.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Debugger.h>

#include "RmlUi_Platform_Win32.h"

#include <cstdio>
#include <memory>
#include <string>

// Mirror of MPGameApp.h mouse flags (avoid pulling DirectInput headers here).
#ifndef M_LDown
#define M_LDown 0x0001
#define M_MDown 0x0002
#define M_RDown 0x0004
#define M_LUp 0x0010
#define M_MUp 0x0020
#define M_RUp 0x0040
#define M_Move 0x0080
#endif

namespace {

class FileInterface_Game : public Rml::FileInterface {
public:
	Rml::FileHandle Open(const Rml::String& path) override {
		// Prefer the raw path first (absolute Windows font paths, etc.).
		FILE* fp = nullptr;
		if (fopen_s(&fp, path.c_str(), "rb") == 0 && fp)
			return reinterpret_cast<Rml::FileHandle>(fp);

		static const char* prefixes[] = {
			"ui/rml/",
			"./ui/rml/",
			"../ui/rml/",
		};

		for (const char* prefix : prefixes) {
			std::string full = std::string(prefix) + path.c_str();
			fp = nullptr;
			if (fopen_s(&fp, full.c_str(), "rb") == 0 && fp)
				return reinterpret_cast<Rml::FileHandle>(fp);
		}
		return 0;
	}

	void Close(Rml::FileHandle file) override {
		if (file)
			fclose(reinterpret_cast<FILE*>(file));
	}

	size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
		return fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
	}

	bool Seek(Rml::FileHandle file, long offset, int origin) override {
		return fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
	}

	size_t Tell(Rml::FileHandle file) override {
		return (size_t)ftell(reinterpret_cast<FILE*>(file));
	}
};

std::unique_ptr<RenderInterface_DX9> g_renderer;
std::unique_ptr<SystemInterface_Win32> g_system;
std::unique_ptr<FileInterface_Game> g_file;
Rml::Context* g_context = nullptr;

int GetKeyModifiers() {
	return RmlWin32::GetKeyModifierState();
}

} // namespace

CRmlUiManager& CRmlUiManager::Instance() {
	static CRmlUiManager instance;
	return instance;
}

bool CRmlUiManager::Init(HWND hwnd) {
	if (m_ready)
		return true;

	m_hwnd = hwnd;
	IDirect3DDevice9* device = g_Render.GetDevice();
	if (!device) {
		OutputDebugStringA("RmlUi: Init failed - no D3D device\n");
		return false;
	}

	g_renderer = std::make_unique<RenderInterface_DX9>();
	g_system = std::make_unique<SystemInterface_Win32>();
	g_file = std::make_unique<FileInterface_Game>();

	g_renderer->SetDevice(device);
	g_system->SetWindow(hwnd);

	Rml::SetRenderInterface(g_renderer.get());
	Rml::SetSystemInterface(g_system.get());
	Rml::SetFileInterface(g_file.get());

	if (!Rml::Initialise()) {
		OutputDebugStringA("RmlUi: Rml::Initialise() failed\n");
		Shutdown();
		return false;
	}

	const int width = g_Render.GetScrWidth();
	const int height = g_Render.GetScrHeight();
	g_renderer->SetViewport(width, height);

	g_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (!g_context) {
		OutputDebugStringA("RmlUi: CreateContext failed\n");
		Shutdown();
		return false;
	}

	// Load fonts before documents. Prefer Windows fonts; also install the debugger's
	// embedded face early so we always have a known-good English font.
	bool font_ok = false;

	auto try_load = [&](const char* path, const char* tag) {
		if (Rml::LoadFontFace(path, true)) {
			font_ok = true;
			char buf[256];
			sprintf_s(buf, "RmlUi: loaded font via %s (%s)\n", tag, path);
			OutputDebugStringA(buf);
			return true;
		}
		char buf[256];
		sprintf_s(buf, "RmlUi: FAILED font %s (%s)\n", tag, path);
		OutputDebugStringA(buf);
		return false;
	};

	try_load("C:\\Windows\\Fonts\\arial.ttf", "arial");
	try_load("C:\\Windows\\Fonts\\segoeui.ttf", "segoeui");
	try_load("C:/Windows/Fonts/arial.ttf", "arial-fwd");
	try_load("ui/rml/fonts/LatoLatin-Regular.ttf", "lato");

	// Install debugger (loads embedded "rmlui-debugger-font") but keep the menu hidden.
	if (Rml::Debugger::Initialise(g_context)) {
		Rml::Debugger::SetVisible(false);
		font_ok = true;
		OutputDebugStringA("RmlUi: debugger font installed (menu hidden)\n");
	}

	if (!font_ok) {
		OutputDebugStringA("RmlUi: ERROR - no font loaded; text will be blank\n");
	}

	// Load the account login replacement (shown by CLoginScene::ShowLoginForm).
	if (!CRmlUiAccountForm::Instance().Load(g_context)) {
		OutputDebugStringA("RmlUi: warning - account.rml failed to load\n");
	}

	m_ready = true;
	OutputDebugStringA("RmlUi: initialized OK\n");
	return true;
}

bool CRmlUiManager::LoadHelloDocument() {
	// Kept for optional debug overlays; not shown by default anymore.
	if (!g_context)
		return false;

	Rml::ElementDocument* document = g_context->LoadDocument("hello.rml");
	if (!document)
		return false;

	document->Hide();
	return true;
}

void CRmlUiManager::Shutdown() {
	m_ready = false;

	CRmlUiAccountForm::Instance().Unload();

	if (g_context) {
		g_context->UnloadAllDocuments();
		g_context = nullptr;
	}

	Rml::Shutdown();

	g_renderer.reset();
	g_system.reset();
	g_file.reset();
	m_hwnd = nullptr;
}

void CRmlUiManager::SyncViewport() {
	if (!m_ready || !g_context || !g_renderer)
		return;

	const int width = g_Render.GetScrWidth();
	const int height = g_Render.GetScrHeight();
	if (width <= 0 || height <= 0)
		return;

	g_renderer->SetViewport(width, height);
	if (g_context->GetDimensions() != Rml::Vector2i(width, height))
		g_context->SetDimensions(Rml::Vector2i(width, height));
}

void CRmlUiManager::Update() {
	if (!m_ready || !m_enabled || !g_context)
		return;

	SyncViewport();
	g_context->Update();
}

void CRmlUiManager::Render() {
	if (!m_ready || !m_enabled || !g_context || !g_renderer)
		return;

	SyncViewport();
	g_renderer->BeginFrame();
	g_context->Render();
	g_renderer->EndFrame();
}

void CRmlUiManager::ProcessMouse(int x, int y, DWORD mouseKey) {
	if (!m_ready || !m_enabled || !g_context)
		return;

	const int mods = GetKeyModifiers();

	if (x != m_lastX || y != m_lastY || (mouseKey & M_Move)) {
		g_context->ProcessMouseMove(x, y, mods);
		m_lastX = x;
		m_lastY = y;
	}

	if ((mouseKey & M_LDown) && !m_leftDown) {
		g_context->ProcessMouseButtonDown(0, mods);
		m_leftDown = true;
	}
	if ((mouseKey & M_LUp) && m_leftDown) {
		g_context->ProcessMouseButtonUp(0, mods);
		m_leftDown = false;
	}

	if ((mouseKey & M_RDown) && !m_rightDown) {
		g_context->ProcessMouseButtonDown(1, mods);
		m_rightDown = true;
	}
	if ((mouseKey & M_RUp) && m_rightDown) {
		g_context->ProcessMouseButtonUp(1, mods);
		m_rightDown = false;
	}

	if ((mouseKey & M_MDown) && !m_middleDown) {
		g_context->ProcessMouseButtonDown(2, mods);
		m_middleDown = true;
	}
	if ((mouseKey & M_MUp) && m_middleDown) {
		g_context->ProcessMouseButtonUp(2, mods);
		m_middleDown = false;
	}
}

void CRmlUiManager::ProcessMouseWheel(int nScroll) {
	if (!m_ready || !m_enabled || !g_context)
		return;

	// Game scroll sign: positive typically means wheel up; RmlUi wants down-positive.
	const float delta = (nScroll > 0) ? -1.0f : 1.0f;
	g_context->ProcessMouseWheel(delta, GetKeyModifiers());
}

bool CRmlUiManager::ProcessKeyDown(int win32Key) {
	if (!m_ready || !m_enabled || !g_context)
		return false;

	// Toggle debugger with F8
	if (win32Key == VK_F8) {
		Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
		return true;
	}

	const Rml::Input::KeyIdentifier key = RmlWin32::ConvertKey(win32Key);
	// ProcessKeyDown returns true if NOT consumed.
	return !g_context->ProcessKeyDown(key, GetKeyModifiers());
}

bool CRmlUiManager::ProcessKeyUp(int win32Key) {
	if (!m_ready || !m_enabled || !g_context)
		return false;

	const Rml::Input::KeyIdentifier key = RmlWin32::ConvertKey(win32Key);
	return !g_context->ProcessKeyUp(key, GetKeyModifiers());
}

bool CRmlUiManager::ProcessTextInput(char character) {
	if (!m_ready || !m_enabled || !g_context)
		return false;

	// Match RmlUi's Win32 backend: only printable characters (plus newline).
	// Control chars like Backspace (\b) / Delete must not be inserted as text —
	// those are handled by ProcessKeyDown (KI_BACK / KI_DELETE). Feeding \b here
	// cancels the delete (delete one char, then insert \b).
	const auto c = static_cast<unsigned char>(character);
	if ((c < 32 && character != '\n') || c == 127) {
		// Still consume when RmlUi has focus so the legacy form stack ignores it.
		return g_context->GetFocusElement() != nullptr;
	}

	return !g_context->ProcessTextInput(character);
}

bool CRmlUiManager::IsMouseInteracting() const {
	if (!m_ready || !m_enabled || !g_context)
		return false;
	return g_context->IsMouseInteracting();
}
