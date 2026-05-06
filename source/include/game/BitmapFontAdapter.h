//----------------------------------------------------------------------
// BitmapFontAdapter.h -> UIFont.h (Bitmap Font Implementation)
// 
// This is the permanent bitmap font system. The old GDI-based CMPFont
// system has been archived.
// 
// Original CGuiFont class has been replaced with this bitmap font
// implementation that provides the same interface.
//----------------------------------------------------------------------
#pragma once

#include "engine/BitmapFont.h"
#include <vector>
#include <string>

namespace GUI {
//----------------------------------------------------------------------
// Font configuration for bitmap fonts
// Maps old-style font creation to bitmap font file paths
//----------------------------------------------------------------------
struct BitmapFontConfig {
    std::string basePath;           // Base path for font files (e.g., "font/")
    std::string defaultFontFile;    // Default .fnt file (e.g., "default.fnt")
    
    BitmapFontConfig() 
        : basePath("font/")
        , defaultFontFile("default.fnt") 
    {}
};

//----------------------------------------------------------------------
// CGuiFont - Bitmap Font Implementation
// 
// This class uses pre-baked bitmap fonts (BMFont/AngelCode format)
// for efficient GPU-accelerated text rendering.
// 
// Provides the same interface as the original GDI-based CGuiFont class
// for backward compatibility with all existing game code.
//----------------------------------------------------------------------
class CGuiFont {
public:
    CGuiFont();
    ~CGuiFont();
    
    //------------------------------------------------------------------
    // Initialization - Bitmap Font Style
    //------------------------------------------------------------------
    
    // Configure the font system
    void SetConfig(const BitmapFontConfig& config);
    
    // Initialize with default configuration
    bool Init();
    
    // Clear all fonts
    bool Clear();
    
    //------------------------------------------------------------------
    // Font Creation
    // For bitmap fonts, this maps to loading pre-defined .fnt files
    // The font name is used to look up the corresponding .fnt file
    //------------------------------------------------------------------
    
    // Load a bitmap font by name (looks for "font/{name}_{size}.fnt")
    // size800/size1024 are used to select appropriate pre-baked size
    int CreateFont(const char* font, int size800, int size1024, DWORD dwStyle = 0);
    
    // Direct font file loading
    int LoadBitmapFont(const char* fntFile);
    
    //------------------------------------------------------------------
    // Screen Settings
    //------------------------------------------------------------------
    
    void SetScreen(int nScrWidth, int nScrHeight);
    void SetIndex(unsigned int n);
    
    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------
    
    // Basic text rendering
    void Render(const char* str, int x, int y, DWORD color, float size = 1.0f);
    
    // Scaled rendering
    void RenderScale(const char* str, int x, int y, DWORD color, float scale);
    
    // Shadow rendering (drop shadow)
    void BRender(const char* str, int x, int y, DWORD color, DWORD shadow);
    
    // Outline/stroke rendering (text with outline around it)
    void ORender(const char* str, int x, int y, DWORD color, DWORD outline, int thickness = 1);
    
    // 3D world-space text (projects to screen)
    void Render3d(const char* str, D3DXVECTOR3& pos, DWORD color = 0xFFFF0000);
    
    // Frame rendering (with background)
    void FrameRender(const char* str, int x, int y);
    
    // Tip rendering (centered with background)
    void TipRender(const char* str, int x, int y);
    
    //------------------------------------------------------------------
    // Text Measurement
    //------------------------------------------------------------------
    
    int GetWidth(const char* str);
    int GetHeight(const char* str);
    void GetSize(const char* str, int& w, int& h);
    bool GetSize(unsigned int dwIndex, const char* str, int& w, int& h);
    
    //------------------------------------------------------------------
    // Batch Rendering
    //------------------------------------------------------------------
    
    void Begin();
    void Draw(const char* str, int x, int y, DWORD color);
    void End();
    
    //------------------------------------------------------------------
    // Font Access (for indexed rendering)
    //------------------------------------------------------------------
    
    BitmapFontSystem::CBitmapFont* GetFont(unsigned int index = 0);
    void Render(unsigned int nIndex, const char* str, int x, int y, DWORD color, float size = 1.0f);
    void BRender(unsigned int nIndex, const char* str, int x, int y, DWORD color, DWORD shadow);
    void ORender(unsigned int nIndex, const char* str, int x, int y, DWORD color, DWORD outline, int thickness = 1);
    
    //------------------------------------------------------------------
    // Check if initialized
    //------------------------------------------------------------------
    bool IsInitialized() const { return !m_Fonts.empty(); }
    
    //------------------------------------------------------------------
    // Pre-baked outline support
    // When true, Render() is used instead of ORender() because the
    // outline is already baked into the font texture (9x faster)
    //------------------------------------------------------------------
    void SetHasPrebakedOutline(bool value) { m_bHasPrebakedOutline = value; }
    bool HasPrebakedOutline() const { return m_bHasPrebakedOutline; }
    
public:
    // Global font instances
    static CGuiFont s_Font;      // Default UI font
    static CGuiFont s_NameFont;  // Player/NPC name font (larger, bold)
    
    // Global flag for name font outline mode
    // Set from font.clu NAME_FONT_HAS_OUTLINE variable
    static bool s_NameFontHasPrebakedOutline;
    
private:
    // Font storage
    typedef std::vector<BitmapFontSystem::CBitmapFont*> FontList;
    FontList m_Fonts;
    
    // Current font index
    unsigned int m_nCurrentIndex;
    BitmapFontSystem::CBitmapFont* m_pCurrentFont;
    
    // Configuration
    BitmapFontConfig m_Config;
    
    // Screen dimensions for scaling
    int m_nScreenWidth;
    int m_nScreenHeight;
    
    // Font info storage (for compatibility)
    struct FontInfo {
        std::string strFont;
        DWORD dwStyle;
        int size800;
        int size1024;
        std::string fntFile;
    };
    std::vector<FontInfo> m_FontInfos;
    
    // Batch rendering state
    bool m_bBatching;
    
    // Pre-baked outline flag
    bool m_bHasPrebakedOutline;
    
    // Helper to get D3D device
    IDirect3DDeviceX* GetDevice();
};

} // namespace GUI
