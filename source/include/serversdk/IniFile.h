#ifndef INIFILE_H
#define INIFILE_H

#include "excp.h"
#include "dstring.h"

_DBC_BEGIN
#pragma pack(push)
#pragma pack(4)

struct IniItem {
	std::string name;
	std::string value;
};
class IniSection {
	friend class IniFile;

public:
	int ItemCount() { return m_itemcount; }
	IniItem& operator[](int);
	std::string& operator[](const char* name) const;

	IniItem* GetItem(const char* name);

private:
	IniSection() : m_itemcount(0), m_itemsize(0), m_item(0) {}
	~IniSection();

	IniItem* AddItem(const char* name, const char* value);

	int m_itemcount;
	int m_itemsize;
	IniItem** m_item;
	std::string m_sectname;
};

class IniFile {
public:
	IniFile(const char* filename);
	~IniFile();

	int SectCount() { return m_sectcount; }
	IniSection& operator[](int);
	IniSection& operator[](const char* sectname) const;

	IniSection* GetSection(const char* sectname);

private:
	void Flush();
	void ReadFile(const char* filename);
	IniSection* AddSection(const char* sectname);

	int m_sectcount;
	int m_sectsize;
	IniSection** m_sect;
	bool m_update;
	bool m_rw; // false:ֻ��;true:����д��
	char m_filename[512];
};

#pragma pack(pop)
_DBC_END

#endif
