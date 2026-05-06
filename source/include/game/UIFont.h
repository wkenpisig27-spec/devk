//----------------------------------------------------------------------
// UIFont.h - GUI Font System (Bitmap Font Implementation)
// 
// Original: lh 2004-07-15
// Bitmap Font Refactor: 2024
//
// This file now uses the bitmap font system exclusively.
// Old GDI-based CMPFont code has been archived.
//----------------------------------------------------------------------
#pragma once

// Always include compcommand.h for enumEQUIP_NUM and other constants
#include "compcommand.h"

// Use bitmap font system (includes CGuiFont class)
#include "BitmapFontAdapter.h"

// UIRender.h is still needed for GetRender() and other functions
#include "UIRender.h"

namespace GUI {
const DWORD COLOR_BLACK = 0xff000000;
const DWORD COLOR_WHITE = 0xffffffff;
const DWORD COLOR_RED = 0xffff0000;
const DWORD COLOR_GREEN = 0xff00ff00;
const DWORD COLOR_BLUE = 0xff0000ff;
const DWORD COLOR_GRAY = 0xff808080;
const DWORD COLOR_CHAR_NAME = COLOR_WHITE;
const DWORD COLOR_SHIP_NAME = 0xff03fdbd;
const DWORD COLOR_SHIP_SEPARATOR = 0xff16074f;
const DWORD COLOR_ORANGE = 0xffffa502;
const DWORD COLOR_YELLOW = 0xffffff00;
#define SCROLLNEEDSPACE 60 // Original: 60

enum ALLIGN { eAlignTop,
			  eAlignCenter,
			  eAlignBottom };

// CGuiFont class is now defined in BitmapFontAdapter.h

// ��ʾ�ɾ��е�hint list
// ʹ�÷���:
// 1.Clear���ԭ�е������б�
// 2.PushHint,AddHintHeight,������б�
// 3.ReadyForHint( x, y ) - ׼�����Ƕ���ʾ
// 4.Render - ��ʾhint
class CTextHint {
public:
	void Clear();
	void ReadyForHint(int x, int y);
	void ReadyForHintGM(int x, int y); // Add by sunny.sun 20080912
	void Render();

	void PushHint(const char* str, DWORD color = COLOR_WHITE, int height = 5, int font = 0, int index = -1, bool shadow = false, DWORD scolor = 0); // height:�۳��ָߺ�ĸ߶�
	void AddHintHeight(int height = 6);																												// ���һ������

	int WriteText(const char* file); // ����ǰhintд���ı�������д�������

public:
	struct stHint {
		stHint(const char* str, DWORD c, int h, int f, bool sh, unsigned int sc)
			: hint(str), color(c), height(h), offx(0), font(f), width(0), shadow(sh), scolor(sc) {}

		std::string hint;
		DWORD color;
		int height;
		int offx;
		int width;
		int font;
		bool shadow;
		unsigned int scolor;
	};

	CTextHint();

	// ��������Hint�Ƿ����
	void SetHintIsCenter(bool v) { _IsCenter = v; }
	bool GetHintIsCenter() { return _IsCenter; }

	void SetIsHeadShadow(bool v) { _IsHeadShadow = v; }
	bool GetIsHeadShadow() { return _IsHeadShadow; }

	stHint* GetHint(int v) { return _hint[v]; }
	stHint* GetHintRelated(int i, int j) { return _hint_related[i][j]; }
	int GetCount() { return (int)_hint.size(); }

	void SetFixWidth(int w);
	int GetFixWidth() { return _nFixWidth; }

	void SetBkgColor(DWORD color) { _dwBkgColor = color; }
	DWORD GetBkgColor() { return _dwBkgColor; }

	void SetIsShowFrame(bool v) { _IsShowFrame = v; }
	bool GetIsShowFrame() { return _IsShowFrame; }

private:
	typedef std::vector<stHint*> hints;
	hints _hint;
	hints _hint_related[enumEQUIP_NUM];

private:
	enum eStyle {
		enumFontSize,
		enumFixWidth,
	};
	eStyle _eStyle;
	int _nFixWidth;

	bool _IsCenter;
	bool _IsHeadShadow;
	bool _IsShowFrame;

	int _nHintW, _nHintH;
	int _nRelatedHintW[enumEQUIP_NUM];
	int _nRelatedHintH[enumEQUIP_NUM];
	int _nStartX, _nStartY;
	DWORD _dwBkgColor;
};

// Add by sunny.sun20080804
// Begin
class CTextScrollHint {
public:
	void Clear();
	void ReadyForHint(int x, int y, int SetNum);
	void Render();

	void PushHint(const char* str, DWORD color = COLOR_WHITE, int height = 5, int font = 0); // height:�۳��ָߺ�ĸ߶�
public:
	struct stHint {
		stHint(const char* str, DWORD c, int h, int f) : hint(str), color(c), height(h), offx(0), font(f), width(0) {}

		std::string hint;
		DWORD color;
		int height;
		int offx;
		int width;
		int font;
	};

	CTextScrollHint();

	void SetFixWidth(int w);
	void SetBkgColor(DWORD color) { _dwBkgColor = color; }

private:
	typedef std::vector<stHint*> hints;
	hints _hint;
	int index;
	std::string CopyHints;
	std::string SpaceLength;
	int num;
	int RenderScrollNum;
	int SetScrollNum;
	int m;

private:
	enum eStyle {
		enumFontSize,
		enumFixWidth,
	};
	eStyle _eStyle;
	int _nFixWidth;

	bool _IsCenter;
	bool _IsHeadShadow;
	bool _IsShowFrame;

	int _nHintW, _nHintH;
	int _nStartX, _nStartY;
	DWORD _dwBkgColor;
};
// End

// CTextHint inline implementations
inline void CTextHint::PushHint(const char* str, DWORD color, int height, int font, int index, bool shadow, DWORD scolor) {
	if (str && strlen(str) > 0) {
		if (index == -1)
			_hint.push_back(new stHint(str, color, height, font, shadow, scolor));
		else
			_hint_related[index].push_back(new stHint(str, color, height, font, shadow, scolor));
	}
}

inline void CTextHint::AddHintHeight(int height) {
	if (!_hint.empty()) {
		_hint.back()->height += height;
	}
}

inline void CTextHint::SetFixWidth(int w) {
	if (w <= 0) {
		_eStyle = enumFontSize;
	} else {
		_nFixWidth = w;
		_eStyle = enumFixWidth;
	}
}
// ��������//Add by sunny.sun20080804
// Begin
inline void CTextScrollHint::PushHint(const char* str, DWORD color, int height, int font) {
	if (str && strlen(str) > 0)
		_hint.push_back(new stHint(str, color, height, font));
}

inline void CTextScrollHint::SetFixWidth(int w) {
	if (w <= 0) {
		_eStyle = enumFontSize;
	} else {
		_nFixWidth = w;
		_eStyle = enumFixWidth;
	}
}
// End
} // namespace GUI
