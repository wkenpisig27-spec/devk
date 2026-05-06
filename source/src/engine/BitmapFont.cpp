//----------------------------------------------------------------------
// Bitmap Font System - Implementation
// 
// Supports BMFont text format (.fnt files)
// https://www.angelcode.com/products/bmfont/doc/file_format.html
//----------------------------------------------------------------------
#include "StdAfx.h"
#include "BitmapFont.h"
#include "MPRender.h"
#include "MPFont.h"  // For ui::UIClip

#include <fstream>
#include <sstream>
#include <cstring>

namespace BitmapFontSystem {

//----------------------------------------------------------------------
// CBitmapFont Implementation
//----------------------------------------------------------------------

CBitmapFont::CBitmapFont()
    : m_bLoaded(false)
    , m_nFontSize(0)
    , m_bBold(false)
    , m_bItalic(false)
    , m_nLineHeight(0)
    , m_nBase(0)
    , m_nScaleW(0)
    , m_nScaleH(0)
    , m_nPages(0)
    , m_pDevice(nullptr)
    , m_nVertexCount(0)
    , m_nCurrentPage(0)
    , m_bInBatch(false)
{
}

CBitmapFont::~CBitmapFont()
{
    Release();
}

void CBitmapFont::Release()
{
    // Release all textures
    for (auto tex : m_Textures) {
        if (tex) {
            tex->Release();
        }
    }
    m_Textures.clear();
    
    m_Glyphs.clear();
    m_Kerning.clear();
    
    m_bLoaded = false;
    m_pDevice = nullptr;
}

//----------------------------------------------------------------------
// Loading
//----------------------------------------------------------------------

bool CBitmapFont::Load(const char* fontFile, const char* texturePath, MPRender* pRender)
{
    if (!pRender) return false;
    return Load(fontFile, texturePath, pRender->GetDevice());
}

bool CBitmapFont::Load(const char* fontFile, const char* texturePath, IDirect3DDeviceX* pDevice)
{
    if (!pDevice || !fontFile) return false;
    
    // Release any existing data
    Release();
    
    m_pDevice = pDevice;
    
    // Open .fnt file
    std::ifstream file(fontFile);
    if (!file.is_open()) {
        OutputDebugStringA("BitmapFont: Failed to open font file: ");
        OutputDebugStringA(fontFile);
        OutputDebugStringA("\n");
        return false;
    }
    
    // Parse line by line
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // Determine line type and parse
        if (line.compare(0, 5, "info ") == 0) {
            ParseInfo(line.c_str());
        }
        else if (line.compare(0, 7, "common ") == 0) {
            ParseCommon(line.c_str());
        }
        else if (line.compare(0, 5, "page ") == 0) {
            ParsePage(line.c_str(), texturePath, pDevice);
        }
        else if (line.compare(0, 5, "char ") == 0) {
            ParseChar(line.c_str());
        }
        else if (line.compare(0, 8, "kerning ") == 0) {
            ParseKerning(line.c_str());
        }
        // Skip "chars count=X" and "kernings count=X" lines
    }
    
    file.close();
    
    // Validate loading
    if (m_Textures.empty() || m_Glyphs.empty()) {
        OutputDebugStringA("BitmapFont: No textures or glyphs loaded\n");
        Release();
        return false;
    }
    
    m_bLoaded = true;
    
    char debugMsg[256];
    sprintf_s(debugMsg, "BitmapFont: Loaded '%s' with %d glyphs, %d pages\n",
              m_strFontName.c_str(), (int)m_Glyphs.size(), (int)m_Textures.size());
    OutputDebugStringA(debugMsg);
    
    return true;
}

//----------------------------------------------------------------------
// Parsing helpers
//----------------------------------------------------------------------

bool CBitmapFont::GetIntValue(const char* line, const char* key, int& value)
{
    // Find "key=" in line
    char searchKey[64];
    sprintf_s(searchKey, "%s=", key);
    
    const char* pos = strstr(line, searchKey);
    if (!pos) return false;
    
    pos += strlen(searchKey);
    value = atoi(pos);
    return true;
}

bool CBitmapFont::GetStringValue(const char* line, const char* key, char* value, int maxLen)
{
    // Find "key=" in line
    char searchKey[64];
    sprintf_s(searchKey, "%s=", key);
    
    const char* pos = strstr(line, searchKey);
    if (!pos) return false;
    
    pos += strlen(searchKey);
    
    // Check if quoted
    if (*pos == '"') {
        pos++;
        const char* end = strchr(pos, '"');
        if (!end) return false;
        
        int len = (int)(end - pos);
        if (len >= maxLen) len = maxLen - 1;
        strncpy_s(value, maxLen, pos, len);
        value[len] = '\0';
    }
    else {
        // Unquoted - read until space or end
        int i = 0;
        while (*pos && *pos != ' ' && *pos != '\t' && i < maxLen - 1) {
            value[i++] = *pos++;
        }
        value[i] = '\0';
    }
    
    return true;
}

bool CBitmapFont::ParseInfo(const char* line)
{
    // info face="Arial" size=32 bold=0 italic=0 ...
    char face[128] = {0};
    GetStringValue(line, "face", face, sizeof(face));
    m_strFontName = face;
    
    GetIntValue(line, "size", m_nFontSize);
    
    int bold = 0, italic = 0;
    GetIntValue(line, "bold", bold);
    GetIntValue(line, "italic", italic);
    m_bBold = (bold != 0);
    m_bItalic = (italic != 0);
    
    return true;
}

bool CBitmapFont::ParseCommon(const char* line)
{
    // common lineHeight=32 base=26 scaleW=256 scaleH=256 pages=1 ...
    GetIntValue(line, "lineHeight", m_nLineHeight);
    GetIntValue(line, "base", m_nBase);
    GetIntValue(line, "scaleW", m_nScaleW);
    GetIntValue(line, "scaleH", m_nScaleH);
    GetIntValue(line, "pages", m_nPages);
    
    return true;
}

bool CBitmapFont::ParsePage(const char* line, const char* texturePath, IDirect3DDeviceX* pDevice)
{
    // page id=0 file="arial_0.png"
    int pageId = 0;
    char fileName[256] = {0};
    
    GetIntValue(line, "id", pageId);
    GetStringValue(line, "file", fileName, sizeof(fileName));
    
    // Build full path
    char fullPath[512];
    sprintf_s(fullPath, "%s%s", texturePath, fileName);
    
    // Load texture using D3DX
    IDirect3DTextureX* pTexture = nullptr;
    HRESULT hr = D3DXCreateTextureFromFileExA(
        pDevice,
        fullPath,
        D3DX_DEFAULT_NONPOW2,   // Width
        D3DX_DEFAULT_NONPOW2,   // Height
        1,                       // MipLevels
        0,                       // Usage
        D3DFMT_A8R8G8B8,        // Format (preserve alpha!)
        D3DPOOL_MANAGED,        // Pool
        D3DX_FILTER_NONE,       // Filter
        D3DX_FILTER_NONE,       // MipFilter
        0,                       // ColorKey (no transparency key)
        nullptr,                // SrcInfo
        nullptr,                // Palette
        &pTexture
    );
    
    if (FAILED(hr) || !pTexture) {
        OutputDebugStringA("BitmapFont: Failed to load texture: ");
        OutputDebugStringA(fullPath);
        OutputDebugStringA("\n");
        return false;
    }
    
    // Ensure vector is large enough
    while ((int)m_Textures.size() <= pageId) {
        m_Textures.push_back(nullptr);
    }
    m_Textures[pageId] = pTexture;
    
    return true;
}

bool CBitmapFont::ParseChar(const char* line)
{
    // char id=65 x=0 y=0 width=17 height=24 xoffset=0 yoffset=6 xadvance=17 page=0 chnl=15
    Glyph g = {0};
    
    int id = 0, x = 0, y = 0, w = 0, h = 0;
    int xoff = 0, yoff = 0, xadv = 0, page = 0;
    
    GetIntValue(line, "id", id);
    GetIntValue(line, "x", x);
    GetIntValue(line, "y", y);
    GetIntValue(line, "width", w);
    GetIntValue(line, "height", h);
    GetIntValue(line, "xoffset", xoff);
    GetIntValue(line, "yoffset", yoff);
    GetIntValue(line, "xadvance", xadv);
    GetIntValue(line, "page", page);
    
    g.charId = (WORD)id;
    g.width = (float)w;
    g.height = (float)h;
    g.xoffset = (float)xoff;
    g.yoffset = (float)yoff;
    g.xadvance = (float)xadv;
    g.page = page;
    
    // Calculate UV coordinates directly (x,y not stored)
    if (m_nScaleW > 0 && m_nScaleH > 0) {
        g.u1 = (float)x / (float)m_nScaleW;
        g.v1 = (float)y / (float)m_nScaleH;
        g.u2 = (float)(x + w) / (float)m_nScaleW;
        g.v2 = (float)(y + h) / (float)m_nScaleH;
    }
    
    m_Glyphs[(WORD)id] = g;
    
    return true;
}

bool CBitmapFont::ParseKerning(const char* line)
{
    // kerning first=65 second=86 amount=-2
    int first = 0, second = 0, amount = 0;
    
    GetIntValue(line, "first", first);
    GetIntValue(line, "second", second);
    GetIntValue(line, "amount", amount);
    
    DWORD key = ((DWORD)first << 16) | (DWORD)second;
    m_Kerning[key] = (float)amount;
    
    return true;
}

//----------------------------------------------------------------------
// Character access
//----------------------------------------------------------------------

WORD CBitmapFont::GetNextChar(const char** pText)
{
    const unsigned char* text = (const unsigned char*)*pText;
    WORD charCode;
    
    // Check for MBCS (Chinese) character
    if (*text & 0x80) {
        // Two-byte character (MBCS/GB2312/GBK)
        charCode = ((WORD)text[0] << 8) | text[1];
        *pText += 2;
    }
    else {
        // Single-byte ASCII
        charCode = text[0];
        *pText += 1;
    }
    
    return charCode;
}

bool CBitmapFont::HasGlyph(WORD charCode) const
{
    return m_Glyphs.find(charCode) != m_Glyphs.end();
}

const Glyph* CBitmapFont::GetGlyph(WORD charCode) const
{
    auto it = m_Glyphs.find(charCode);
    if (it != m_Glyphs.end()) {
        return &it->second;
    }
    return nullptr;
}

float CBitmapFont::GetKerning(WORD first, WORD second) const
{
    DWORD key = ((DWORD)first << 16) | (DWORD)second;
    auto it = m_Kerning.find(key);
    if (it != m_Kerning.end()) {
        return it->second;
    }
    return 0.0f;
}

//----------------------------------------------------------------------
// Text measurement
//----------------------------------------------------------------------

int CBitmapFont::GetTextWidth(const char* text) const
{
    if (!text || !m_bLoaded) return 0;
    
    float width = 0.0f;
    float maxWidth = 0.0f;
    WORD prevChar = 0;
    
    while (*text) {
        if (*text == '\n') {
            if (width > maxWidth) maxWidth = width;
            width = 0.0f;
            prevChar = 0;
            text++;
            continue;
        }
        
        WORD charCode = GetNextChar(&text);
        const Glyph* g = GetGlyph(charCode);
        
        if (g) {
            // Add kerning if applicable
            if (prevChar != 0) {
                width += GetKerning(prevChar, charCode);
            }
            width += g->xadvance;
            prevChar = charCode;
        }
    }
    
    return (int)(width > maxWidth ? width : maxWidth);
}

int CBitmapFont::GetTextHeight(const char* text) const
{
    if (!text || !m_bLoaded) return m_nLineHeight;
    
    int lines = 1;
    while (*text) {
        if (*text == '\n') lines++;
        text++;
    }
    
    return lines * m_nLineHeight;
}

void CBitmapFont::GetTextSize(const char* text, int& width, int& height) const
{
    width = GetTextWidth(text);
    height = GetTextHeight(text);
}

//----------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------

void CBitmapFont::AddQuad(float x, float y, float w, float h,
                          float u1, float v1, float u2, float v2,
                          DWORD color)
{
    if (m_nVertexCount + 6 > MAX_VERTICES) {
        // Buffer full - flush current batch
        FlushBatch(m_nCurrentPage);
    }
    
    // Check clip rect - if clipping is enabled, skip characters outside the clip area
    ui::UIClip* pClip = ui::UIClip::GetCliper();
    if (pClip && pClip->GetClipState() == UI_STATE_CLIP) {
        RECT& clipRect = pClip->GetClipRect();
        
        // If character is completely outside clip rect, skip it
        if (x + w < clipRect.left || x > clipRect.right ||
            y + h < clipRect.top || y > clipRect.bottom) {
            return;
        }
        
        // If character is partially clipped, adjust the quad
        float origX = x, origY = y, origW = w, origH = h;
        float origU1 = u1, origV1 = v1, origU2 = u2, origV2 = v2;
        
        // Clip left
        if (x < clipRect.left) {
            float clipAmount = (clipRect.left - x) / w;
            x = (float)clipRect.left;
            u1 = origU1 + (origU2 - origU1) * clipAmount;
            w = origW - (clipRect.left - origX);
        }
        
        // Clip right
        if (x + w > clipRect.right) {
            float clipAmount = ((x + w) - clipRect.right) / origW;
            w = (float)clipRect.right - x;
            u2 = origU2 - (origU2 - origU1) * clipAmount;
        }
        
        // Clip top
        if (y < clipRect.top) {
            float clipAmount = (clipRect.top - y) / h;
            y = (float)clipRect.top;
            v1 = origV1 + (origV2 - origV1) * clipAmount;
            h = origH - (clipRect.top - origY);
        }
        
        // Clip bottom
        if (y + h > clipRect.bottom) {
            float clipAmount = ((y + h) - clipRect.bottom) / origH;
            h = (float)clipRect.bottom - y;
            v2 = origV2 - (origV2 - origV1) * clipAmount;
        }
        
        // If clipping resulted in zero or negative size, skip
        if (w <= 0 || h <= 0) {
            return;
        }
    }
    
    // Pixel-perfect positioning offset
    const float offset = -0.5f;
    
    // Two triangles forming a quad (6 vertices)
    // Triangle 1: top-left, top-right, bottom-left
    m_Vertices[m_nVertexCount].x = x + offset;
    m_Vertices[m_nVertexCount].y = y + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u1;
    m_Vertices[m_nVertexCount].v = v1;
    m_nVertexCount++;
    
    m_Vertices[m_nVertexCount].x = x + w + offset;
    m_Vertices[m_nVertexCount].y = y + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u2;
    m_Vertices[m_nVertexCount].v = v1;
    m_nVertexCount++;
    
    m_Vertices[m_nVertexCount].x = x + offset;
    m_Vertices[m_nVertexCount].y = y + h + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u1;
    m_Vertices[m_nVertexCount].v = v2;
    m_nVertexCount++;
    
    // Triangle 2: top-right, bottom-right, bottom-left
    m_Vertices[m_nVertexCount].x = x + w + offset;
    m_Vertices[m_nVertexCount].y = y + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u2;
    m_Vertices[m_nVertexCount].v = v1;
    m_nVertexCount++;
    
    m_Vertices[m_nVertexCount].x = x + w + offset;
    m_Vertices[m_nVertexCount].y = y + h + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u2;
    m_Vertices[m_nVertexCount].v = v2;
    m_nVertexCount++;
    
    m_Vertices[m_nVertexCount].x = x + offset;
    m_Vertices[m_nVertexCount].y = y + h + offset;
    m_Vertices[m_nVertexCount].z = 0.0f;
    m_Vertices[m_nVertexCount].rhw = 1.0f;
    m_Vertices[m_nVertexCount].color = color;
    m_Vertices[m_nVertexCount].u = u1;
    m_Vertices[m_nVertexCount].v = v2;
    m_nVertexCount++;
}

void CBitmapFont::FlushBatch(int textureIndex)
{
    if (m_nVertexCount == 0 || !m_pDevice) return;
    
    // Set texture
    if (textureIndex >= 0 && textureIndex < (int)m_Textures.size()) {
        m_pDevice->SetTexture(0, m_Textures[textureIndex]);
    }
    
    // Draw triangles
    m_pDevice->SetFVF(FontVertex::FVF);
    m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_nVertexCount / 3, m_Vertices, sizeof(FontVertex));
    
    m_nVertexCount = 0;
}

void CBitmapFont::Begin()
{
    if (!m_pDevice || !m_bLoaded) return;
    
    m_bInBatch = true;
    m_nVertexCount = 0;
    m_nCurrentPage = -1;
    
    // Save render states
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Texture states for proper alpha
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    
    // Sampler states
    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

void CBitmapFont::End()
{
    if (!m_bInBatch) return;
    
    // Flush remaining vertices
    if (m_nVertexCount > 0 && m_nCurrentPage >= 0) {
        FlushBatch(m_nCurrentPage);
    }
    
    m_bInBatch = false;
    
    // Restore render states
    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void CBitmapFont::Draw(const char* text, int x, int y, DWORD color)
{
    if (!text || !m_bLoaded) return;
    
    float cursorX = (float)x;
    float cursorY = (float)y;
    WORD prevChar = 0;
    
    while (*text) {
        // Handle newline
        if (*text == '\n') {
            cursorX = (float)x;
            cursorY += m_nLineHeight;
            prevChar = 0;
            text++;
            continue;
        }
        
        // Get character code
        WORD charCode = GetNextChar(&text);
        const Glyph* g = GetGlyph(charCode);
        
        if (!g) {
            // Character not in font - try fallback to '?' or space
            g = GetGlyph('?');
            if (!g) g = GetGlyph(' ');
            if (!g) continue;
        }
        
        // Handle texture page change
        if (m_nCurrentPage != g->page) {
            // Flush previous page
            if (m_nVertexCount > 0) {
                FlushBatch(m_nCurrentPage);
            }
            m_nCurrentPage = g->page;
        }
        
        // Apply kerning
        if (prevChar != 0) {
            cursorX += GetKerning(prevChar, charCode);
        }
        
        // Add glyph quad
        if (g->width > 0 && g->height > 0) {
            AddQuad(
                cursorX + g->xoffset,
                cursorY + g->yoffset,
                g->width,
                g->height,
                g->u1, g->v1, g->u2, g->v2,
                color
            );
        }
        
        cursorX += g->xadvance;
        prevChar = charCode;
    }
}

void CBitmapFont::DrawText(const char* text, int x, int y, DWORD color)
{
    Begin();
    Draw(text, x, y, color);
    End();
}

void CBitmapFont::DrawTextScaled(const char* text, int x, int y, float scale, DWORD color)
{
    if (!text || !m_bLoaded || scale <= 0.0f) return;
    
    Begin();
    
    float cursorX = (float)x;
    float cursorY = (float)y;
    WORD prevChar = 0;
    
    while (*text) {
        if (*text == '\n') {
            cursorX = (float)x;
            cursorY += m_nLineHeight * scale;
            prevChar = 0;
            text++;
            continue;
        }
        
        WORD charCode = GetNextChar(&text);
        const Glyph* g = GetGlyph(charCode);
        
        if (!g) continue;
        
        if (m_nCurrentPage != g->page) {
            if (m_nVertexCount > 0) {
                FlushBatch(m_nCurrentPage);
            }
            m_nCurrentPage = g->page;
        }
        
        if (prevChar != 0) {
            cursorX += GetKerning(prevChar, charCode) * scale;
        }
        
        if (g->width > 0 && g->height > 0) {
            AddQuad(
                cursorX + g->xoffset * scale,
                cursorY + g->yoffset * scale,
                g->width * scale,
                g->height * scale,
                g->u1, g->v1, g->u2, g->v2,
                color
            );
        }
        
        cursorX += g->xadvance * scale;
        prevChar = charCode;
    }
    
    End();
}

void CBitmapFont::DrawTextShadow(const char* text, int x, int y, DWORD color, DWORD shadowColor)
{
    Begin();
    
    // Draw shadow (offset by 1 pixel)
    Draw(text, x + 1, y + 1, shadowColor);
    
    // Flush shadow
    if (m_nVertexCount > 0) {
        FlushBatch(m_nCurrentPage);
    }
    
    // Draw main text
    Draw(text, x, y, color);
    
    End();
}

void CBitmapFont::DrawTextOutline(const char* text, int x, int y, DWORD color, DWORD outlineColor, int thickness)
{
    Begin();
    
    // Optimized path for thickness=1 (most common case)
    // Draw only the 8 cardinal/diagonal directions instead of full grid
    if (thickness == 1) {
        // Cardinal directions (4)
        Draw(text, x - 1, y, outlineColor);
        Draw(text, x + 1, y, outlineColor);
        Draw(text, x, y - 1, outlineColor);
        Draw(text, x, y + 1, outlineColor);
        // Diagonal directions (4)
        Draw(text, x - 1, y - 1, outlineColor);
        Draw(text, x + 1, y - 1, outlineColor);
        Draw(text, x - 1, y + 1, outlineColor);
        Draw(text, x + 1, y + 1, outlineColor);
    } else {
        // General case for thickness > 1
        for (int dx = -thickness; dx <= thickness; dx++) {
            for (int dy = -thickness; dy <= thickness; dy++) {
                if (dx == 0 && dy == 0) continue; // Skip center
                Draw(text, x + dx, y + dy, outlineColor);
            }
        }
    }
    
    // Flush outline
    if (m_nVertexCount > 0) {
        FlushBatch(m_nCurrentPage);
    }
    
    // Draw main text on top
    Draw(text, x, y, color);
    
    End();
}

} // namespace BitmapFontSystem
