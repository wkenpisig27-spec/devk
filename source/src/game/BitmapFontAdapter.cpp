//----------------------------------------------------------------------
// BitmapFontAdapter.cpp -> UIFont.cpp (Bitmap Font Implementation)
// 
// Implementation of CGuiFont using bitmap fonts (BMFont/AngelCode format)
//----------------------------------------------------------------------
#include "StdAfx.h"
#include "game/BitmapFontAdapter.h"
#include "engine/MPRender.h"
#include "game/UIRender.h"

using namespace GUI;
using namespace BitmapFontSystem;

//----------------------------------------------------------------------
// Static instances
//----------------------------------------------------------------------
CGuiFont CGuiFont::s_Font;
CGuiFont CGuiFont::s_NameFont;
bool CGuiFont::s_NameFontHasPrebakedOutline = false;

//----------------------------------------------------------------------
// Constructor / Destructor
//----------------------------------------------------------------------
CGuiFont::CGuiFont()
    : m_nCurrentIndex(0)
    , m_pCurrentFont(nullptr)
    , m_nScreenWidth(1024)
    , m_nScreenHeight(768)
    , m_bBatching(false)
    , m_bHasPrebakedOutline(false)
{
}

CGuiFont::~CGuiFont() {
    Clear();
}

//----------------------------------------------------------------------
// Configuration
//----------------------------------------------------------------------
void CGuiFont::SetConfig(const BitmapFontConfig& config) {
    m_Config = config;
}

//----------------------------------------------------------------------
// Initialization
//----------------------------------------------------------------------
bool CGuiFont::Init() {
    // On x64, fonts are created via Lua scripts (font.clu) which call UI_CreateFont()
    // Init() just needs to initialize the system, not load a default font
    // Fonts will be loaded on-demand as the Lua scripts create them
    
    // If no fonts loaded yet, try to load default (if it exists)
    if (m_Fonts.empty() && !m_Config.defaultFontFile.empty()) {
        std::string defaultPath = m_Config.basePath + m_Config.defaultFontFile;
        int idx = LoadBitmapFont(defaultPath.c_str());
        // Don't fail if default doesn't exist - fonts will be created by Lua
        if (idx < 0) {
            // Just log and continue - Lua will create fonts via UI_CreateFont()
        }
    }
    
    // Always return true - fonts will be created by Lua scripts
    return true;
}

bool CGuiFont::Clear() {
    for (auto* font : m_Fonts) {
        if (font) {
            font->Release();
            delete font;
        }
    }
    m_Fonts.clear();
    m_FontInfos.clear();
    m_pCurrentFont = nullptr;
    m_nCurrentIndex = 0;
    return true;
}

//----------------------------------------------------------------------
// Font Creation
//----------------------------------------------------------------------
int CGuiFont::CreateFont(const char* font, int size800, int size1024, DWORD dwStyle) {
    // Store font info for compatibility
    FontInfo info;
    info.strFont = font ? font : "";
    info.size800 = size800;
    info.size1024 = size1024;
    info.dwStyle = dwStyle;
    
    // Build .fnt filename
    // The font name IS the filename - no size suffix appended
    // e.g., "gamedefaultsm" -> "font/gamedefaultsm.fnt"
    // e.g., "gamedefaultbold" -> "font/gamedefaultbold.fnt"
    char fntFilename[256];
    if (font && font[0]) {
        sprintf(fntFilename, "%s%s.fnt", m_Config.basePath.c_str(), font);
    } else {
        sprintf(fntFilename, "%s%s", m_Config.basePath.c_str(), m_Config.defaultFontFile.c_str());
    }
    
    info.fntFile = fntFilename;
    m_FontInfos.push_back(info);
    
    // Load the bitmap font
    return LoadBitmapFont(fntFilename);
}

int CGuiFont::LoadBitmapFont(const char* fntFile) {
    IDirect3DDeviceX* pDevice = GetDevice();
    if (!pDevice) {
        return -1;
    }
    
    // Create new bitmap font
    CBitmapFont* pFont = new CBitmapFont();
    
    // Extract base path from fnt file
    std::string texPath = fntFile;
    size_t lastSlash = texPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        texPath = texPath.substr(0, lastSlash + 1);
    } else {
        texPath = "";
    }
    
    // Load the font
    if (!pFont->Load(fntFile, texPath.c_str(), pDevice)) {
        delete pFont;
        return -1;
    }
    
    // Add to font list
    int index = (int)m_Fonts.size();
    m_Fonts.push_back(pFont);
    
    // Set as current if first font
    if (index == 0) {
        m_pCurrentFont = pFont;
        m_nCurrentIndex = 0;
    }
    
    return index;
}

//----------------------------------------------------------------------
// Screen Settings
//----------------------------------------------------------------------
void CGuiFont::SetScreen(int nScrWidth, int nScrHeight) {
    m_nScreenWidth = nScrWidth;
    m_nScreenHeight = nScrHeight;
}

void CGuiFont::SetIndex(unsigned int n) {
    if (n < m_Fonts.size()) {
        m_nCurrentIndex = n;
        m_pCurrentFont = m_Fonts[n];
    }
}

//----------------------------------------------------------------------
// Font Access
//----------------------------------------------------------------------
CBitmapFont* CGuiFont::GetFont(unsigned int index) {
    if (index < m_Fonts.size()) {
        return m_Fonts[index];
    }
    return m_pCurrentFont;
}

//----------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------
void CGuiFont::Render(const char* str, int x, int y, DWORD color, float size) {
    if (!m_pCurrentFont || !str) return;
    
    if (size != 1.0f) {
        m_pCurrentFont->DrawTextScaled(str, x, y, size, color);
    } else {
        m_pCurrentFont->DrawText(str, x, y, color);
    }
}

void CGuiFont::RenderScale(const char* str, int x, int y, DWORD color, float scale) {
    if (!m_pCurrentFont || !str) return;
    m_pCurrentFont->DrawTextScaled(str, x, y, scale, color);
}

void CGuiFont::BRender(const char* str, int x, int y, DWORD color, DWORD shadow) {
    if (!m_pCurrentFont || !str) return;
    m_pCurrentFont->DrawTextShadow(str, x, y, color, shadow);
}

void CGuiFont::ORender(const char* str, int x, int y, DWORD color, DWORD outline, int thickness) {
    if (!m_pCurrentFont || !str) return;
    m_pCurrentFont->DrawTextOutline(str, x, y, color, outline, thickness);
}

void CGuiFont::Render(unsigned int nIndex, const char* str, int x, int y, DWORD color, float size) {
    CBitmapFont* pFont = GetFont(nIndex);
    if (!pFont || !str) return;
    
    if (size != 1.0f) {
        pFont->DrawTextScaled(str, x, y, size, color);
    } else {
        pFont->DrawText(str, x, y, color);
    }
}

void CGuiFont::BRender(unsigned int nIndex, const char* str, int x, int y, DWORD color, DWORD shadow) {
    CBitmapFont* pFont = GetFont(nIndex);
    if (!pFont || !str) return;
    pFont->DrawTextShadow(str, x, y, color, shadow);
}

void CGuiFont::ORender(unsigned int nIndex, const char* str, int x, int y, DWORD color, DWORD outline, int thickness) {
    CBitmapFont* pFont = GetFont(nIndex);
    if (!pFont || !str) return;
    pFont->DrawTextOutline(str, x, y, color, outline, thickness);
}

void CGuiFont::Render3d(const char* str, D3DXVECTOR3& pos, DWORD color) {
    // Note: 3D world-space text rendering is handled by the game's WorldToScreen
    // projection. This method provides a simple NDC-to-screen fallback for
    // pre-projected coordinates. Most game code uses WorldToScreen + regular Render.
    if (!m_pCurrentFont || !str) return;
    
    // Convert normalized device coordinates to screen position
    int x = (int)((pos.x + 1.0f) * 0.5f * m_nScreenWidth);
    int y = (int)((1.0f - pos.y) * 0.5f * m_nScreenHeight);
    m_pCurrentFont->DrawText(str, x, y, color);
}

void CGuiFont::FrameRender(const char* str, int x, int y) {
    if (!m_pCurrentFont || !str) return;
    
    int w, h;
    GetSize(str, w, h);
    
    // Adjust x if would go off-screen
    int offx = m_nScreenWidth - x - w - 10;
    if (offx < 0) {
        x += offx;
    }
    
    // Draw background frame
    GetRender().FillFrame(x - 5, y - 3, x + w + 10, y + h + 5, 0x90000000);
    
    // Draw text
    m_pCurrentFont->DrawText(str, x, y, 0xFFFFFFFF);
}

void CGuiFont::TipRender(const char* str, int x, int y) {
    if (!m_pCurrentFont || !str) return;
    
    int w, h;
    GetSize(str, w, h);
    x -= w / 2;
    
    // Draw background frame
    GetRender().FillFrame(x - 2, y - 1, x + w + 2, y + h + 1, 0x90000000);
    
    // Draw text
    m_pCurrentFont->DrawText(str, x, y, 0xFFFFFFFF);
}

//----------------------------------------------------------------------
// Text Measurement
//----------------------------------------------------------------------
int CGuiFont::GetWidth(const char* str) {
    if (!m_pCurrentFont || !str) return 0;
    return m_pCurrentFont->GetTextWidth(str);
}

int CGuiFont::GetHeight(const char* str) {
    if (!m_pCurrentFont || !str) return 0;
    return m_pCurrentFont->GetTextHeight(str);
}

void CGuiFont::GetSize(const char* str, int& w, int& h) {
    if (!m_pCurrentFont || !str) {
        w = h = 0;
        return;
    }
    m_pCurrentFont->GetTextSize(str, w, h);
}

bool CGuiFont::GetSize(unsigned int dwIndex, const char* str, int& w, int& h) {
    CBitmapFont* pFont = GetFont(dwIndex);
    if (!pFont || !str) {
        w = h = 0;
        return false;
    }
    pFont->GetTextSize(str, w, h);
    return true;
}

//----------------------------------------------------------------------
// Batch Rendering
//----------------------------------------------------------------------
void CGuiFont::Begin() {
    if (m_pCurrentFont) {
        m_pCurrentFont->Begin();
        m_bBatching = true;
    }
}

void CGuiFont::Draw(const char* str, int x, int y, DWORD color) {
    if (m_pCurrentFont && str) {
        m_pCurrentFont->Draw(str, x, y, color);
    }
}

void CGuiFont::End() {
    if (m_pCurrentFont && m_bBatching) {
        m_pCurrentFont->End();
        m_bBatching = false;
    }
}

//----------------------------------------------------------------------
// Helper: Get D3D Device
//----------------------------------------------------------------------
IDirect3DDeviceX* CGuiFont::GetDevice() {
    // Access global render from engine
    extern MINDPOWER_API MPRender g_Render;
    return g_Render.GetDevice();
}
