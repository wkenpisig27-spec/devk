//----------------------------------------------------------------------
// Bitmap Font System
// A fast, pre-baked font renderer using BMFont/AngelCode format
// 
// Author: PKODev Team
// Date: January 2026
//----------------------------------------------------------------------
#pragma once

#include "lwDirectX.h"
#include "MindPowerAPI.h"
#include <map>
#include <string>
#include <vector>

// Forward declarations
class MPRender;

namespace BitmapFontSystem {

//----------------------------------------------------------------------
// Glyph information for a single character
//----------------------------------------------------------------------
struct Glyph {
    WORD charId;        // Character code (Unicode or MBCS)
    
    // Glyph dimensions in pixels
    float width;        // Glyph width
    float height;       // Glyph height
    
    // UV coordinates (0.0 - 1.0)
    float u1, v1;       // Top-left UV
    float u2, v2;       // Bottom-right UV
    
    // Rendering offsets
    float xoffset;      // X offset when rendering
    float yoffset;      // Y offset when rendering
    float xadvance;     // Cursor advance after this glyph
    
    // Texture page (for multi-page fonts)
    int page;
};

//----------------------------------------------------------------------
// Font vertex for rendering
//----------------------------------------------------------------------
struct FontVertex {
    float x, y, z, rhw; // Screen position (transformed)
    DWORD color;        // Vertex color
    float u, v;         // Texture coordinates
    
    static const DWORD FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

//----------------------------------------------------------------------
// CBitmapFont - Main bitmap font class
//----------------------------------------------------------------------
class MINDPOWER_API CBitmapFont {
public:
    CBitmapFont();
    ~CBitmapFont();
    
    //------------------------------------------------------------------
    // Initialization
    //------------------------------------------------------------------
    
    // Load font from .fnt file (BMFont text format)
    // fontFile: Path to .fnt file (e.g., "font/arial.fnt")
    // texturePath: Base path for texture files (e.g., "font/")
    // pDevice: D3D device for texture creation
    bool Load(const char* fontFile, const char* texturePath, IDirect3DDeviceX* pDevice);
    
    // Load font from .fnt file using MPRender
    bool Load(const char* fontFile, const char* texturePath, MPRender* pRender);
    
    // Release all resources
    void Release();
    
    // Check if font is loaded
    bool IsLoaded() const { return m_bLoaded; }
    
    //------------------------------------------------------------------
    // Text Measurement
    //------------------------------------------------------------------
    
    // Get width of text string in pixels
    int GetTextWidth(const char* text) const;
    
    // Get height of text string in pixels (usually line height)
    int GetTextHeight(const char* text) const;
    
    // Get both width and height
    void GetTextSize(const char* text, int& width, int& height) const;
    
    // Get line height
    int GetLineHeight() const { return m_nLineHeight; }
    
    //------------------------------------------------------------------
    // Rendering
    //------------------------------------------------------------------
    
    // Render text at position with color
    void DrawText(const char* text, int x, int y, DWORD color = 0xFFFFFFFF);
    
    // Render text with scale
    void DrawTextScaled(const char* text, int x, int y, float scale, DWORD color = 0xFFFFFFFF);
    
    // Render text with shadow (drop shadow at +1,+1)
    void DrawTextShadow(const char* text, int x, int y, DWORD color, DWORD shadowColor);
    
    // Render text with outline/stroke (draws outline in 8 directions)
    void DrawTextOutline(const char* text, int x, int y, DWORD color, DWORD outlineColor, int thickness = 1);
    
    // Batch rendering (call Begin, multiple Draw calls, then End)
    void Begin();
    void Draw(const char* text, int x, int y, DWORD color = 0xFFFFFFFF);
    void End();
    
    //------------------------------------------------------------------
    // Character Support
    //------------------------------------------------------------------
    
    // Check if a character is available in this font
    bool HasGlyph(WORD charCode) const;
    
    // Get glyph info (returns nullptr if not found)
    const Glyph* GetGlyph(WORD charCode) const;
    
    // Get kerning amount between two characters
    float GetKerning(WORD first, WORD second) const;
    
    //------------------------------------------------------------------
    // Font Properties
    //------------------------------------------------------------------
    
    const std::string& GetFontName() const { return m_strFontName; }
    int GetFontSize() const { return m_nFontSize; }
    int GetBase() const { return m_nBase; }
    int GetTextureWidth() const { return m_nScaleW; }
    int GetTextureHeight() const { return m_nScaleH; }
    int GetPageCount() const { return (int)m_Textures.size(); }
    
private:
    //------------------------------------------------------------------
    // Internal helpers
    //------------------------------------------------------------------
    
    // Parse different section types
    bool ParseInfo(const char* line);
    bool ParseCommon(const char* line);
    bool ParsePage(const char* line, const char* texturePath, IDirect3DDeviceX* pDevice);
    bool ParseChar(const char* line);
    bool ParseKerning(const char* line);
    
    // Extract value from "key=value" pair
    static bool GetIntValue(const char* line, const char* key, int& value);
    static bool GetStringValue(const char* line, const char* key, char* value, int maxLen);
    
    // Get next character from MBCS string, advancing pointer
    static WORD GetNextChar(const char** pText);
    
    // Add a quad to the vertex buffer
    void AddQuad(float x, float y, float w, float h,
                 float u1, float v1, float u2, float v2,
                 DWORD color);
    
    // Flush vertex buffer to GPU
    void FlushBatch(int textureIndex);
    
private:
    //------------------------------------------------------------------
    // Font data
    //------------------------------------------------------------------
    
    bool m_bLoaded;
    
    // Font info (from "info" line)
    std::string m_strFontName;
    int m_nFontSize;
    bool m_bBold;
    bool m_bItalic;
    
    // Common info (from "common" line)
    int m_nLineHeight;      // Line height in pixels
    int m_nBase;            // Baseline offset
    int m_nScaleW;          // Texture width
    int m_nScaleH;          // Texture height
    int m_nPages;           // Number of texture pages
    
    // Glyph data
    std::map<WORD, Glyph> m_Glyphs;
    
    // Kerning data (key = first << 16 | second)
    std::map<DWORD, float> m_Kerning;
    
    // Textures (one per page)
    std::vector<IDirect3DTextureX*> m_Textures;
    
    //------------------------------------------------------------------
    // Rendering state
    //------------------------------------------------------------------
    
    IDirect3DDeviceX* m_pDevice;
    
    // Vertex batching
    static const int MAX_VERTICES = 6000;  // 1000 characters max per batch
    FontVertex m_Vertices[MAX_VERTICES];
    int m_nVertexCount;
    int m_nCurrentPage;     // Current texture page being batched
    
    bool m_bInBatch;        // Are we between Begin/End?
};

} // namespace BitmapFontSystem

// Convenient typedef
using CBitmapFont = BitmapFontSystem::CBitmapFont;
