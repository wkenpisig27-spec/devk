#pragma once

#include "UIFont.h"

namespace GUI {

class CGraph;

class CTextParse {
public:
	struct node // 节点
	{
		std::string str; // 节点数据
		node* next;		 // 指向下一个节点的指针
	};

public:
	CTextParse(void);
	~CTextParse(void);

	void Render(std::string _str, int x, int y, DWORD color, ALLIGN allign = eAlignTop, int height = 0);
	void RenderEx(std::string _str, int x, int y, DWORD color, float scale);

	void RenderPic(std::string _str, int x, int y);

	bool Init();
	int InitEx(std::string _str);
	int InitLink();

	bool Clear();

	void SetBoxOff(char cBoxOff) { _cBoxOff = cBoxOff; }
	char GetBoxOff() { return _cBoxOff; }

	void SetCaption(const char* str) { _str = str; };
	const char* GetCaption() { return _str.c_str(); }

	// 设置表情对应的索引号
	void AddFace(int nIndex, CGraph* pGraph);
	CGraph* GetFace(DWORD nIndex);
	int GetFaceCount() { return (int)_files.size(); }

private:
	std::string _str;

private:
	struct stFaceIndex {
		int nIndex;
		CGraph* pGraph;
	};
	typedef std::vector<stFaceIndex> files;

	char _cBoxOff; // 分隔符,默认为'#'
	files _files;
	int _scaleX;
	int _scaleY;
};

extern CTextParse g_TextParse;

// 算法
// 获取一个字符串的前面n个字符或者后面的n个字符
// flag ==true ,则表示获取前面的字符
inline std::string GetSelfString(std::string str, int n, bool flag) {
	std::string strReturn;
	const char* s = str.c_str();

	if ((int)strlen(s) <= n) {
		strReturn = s;
		return strReturn;
	}

	int i;
	if (flag) {
		i = 0;
		while (i < n) {
			if (s[i] & 0x80)
				i += 2;
			else
				i += 1;
		}
		strReturn = str.substr(0, i);

	} else {
		i = 0;
		int len = (int)str.length() - 1;
		while (i < n) {
			if (s[len - i] & 0x80)
				i += 2;
			else
				i += 1;
		}
		strReturn = str.substr(len + 1 - i, i);
	}

	return strReturn;
}

// Get text that fits within maxPixelWidth, with word-wrapping support
// Returns the portion of text that fits, breaking at word boundaries when possible
// outRemainder: if provided, receives the remaining text after the break point
inline std::string GetStringByPixelWidth(const std::string& str, int maxPixelWidth, std::string* outRemainder = nullptr) {
	if (outRemainder) *outRemainder = "";
	
	if (str.empty() || maxPixelWidth <= 0)
		return "";

	const char* s = str.c_str();
	int len = (int)str.length();
	
	// Check if entire string fits
	int totalWidth = CGuiFont::s_Font.GetWidth(s);
	if (totalWidth <= maxPixelWidth)
		return str;

	int lastSpacePos = -1;
	int lastBreakPos = 0; // Last valid break position
	int i = 0;

	while (i < len) {
		// Handle multi-byte characters (Chinese, etc.)
		int charLen = 1;
		if (s[i] & 0x80) {
			charLen = 2;
		}

		// Remember last space position for word-wrapping
		if (charLen == 1 && s[i] == ' ') {
			lastSpacePos = i;
		}

		// Measure actual substring width (accounts for kerning)
		std::string testStr = str.substr(0, i + charLen);
		int testWidth = CGuiFont::s_Font.GetWidth(testStr.c_str());

		// Check if this substring exceeds width
		if (testWidth > maxPixelWidth) {
			int breakPos;
			
			// If we haven't fit any characters yet, force at least one
			if (i == 0) {
				breakPos = charLen; // Force include at least one character
			}
			// If we have a space to break at, use it (word-wrap)
			else if (lastSpacePos > 0) {
				breakPos = lastSpacePos; // Break before the space
			}
			// Otherwise break at current position
			else {
				breakPos = i;
			}
			
			std::string result = str.substr(0, breakPos);
			if (outRemainder) {
				*outRemainder = str.substr(breakPos);
				// Trim leading spaces from remainder
				while (!outRemainder->empty() && (*outRemainder)[0] == ' ')
					*outRemainder = outRemainder->substr(1);
			}
			return result;
		}

		lastBreakPos = i + charLen;
		i += charLen;
	}

	return str;
}

} // namespace GUI
