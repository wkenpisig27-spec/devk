#pragma once
#include <unicode/resbund.h>
#include <unicode/ucnv.h>
#include <unicode/uclean.h>
#include <unicode/msgfmt.h>
#include <unicode/fmtable.h>
	
//#include "pi_Alloc.h"

#include <map>
#include <memory>

// Forward declarations
using icu::Formattable;
using icu::ResourceBundle;
using icu::UnicodeString;

// Use std::string as key to avoid dangling pointer issues
typedef std::map<std::string, std::unique_ptr<char[]>> StringMap;

class CFormatParameter
{
private:
	Formattable* m_MsgArgs;
	int paraNum;
private:
	CFormatParameter(){}
public:
	CFormatParameter(int paraNum);
	~CFormatParameter();

	Formattable* GetMsgArgs(){ return m_MsgArgs;} 
	int GetParaNum() { return paraNum; }
	void setDouble(int index, double d);
	void  setLong(int index, int32_t l);
	void  setInt64(int index, int64_t ll) ;
	void  setDate(int index, UDate d) ;
	void  setString(int index, const UnicodeString &stringToCopy);
};

class CResourceBundleManage
{
private:
	static CResourceBundleManage* _instance;

public:
	static CResourceBundleManage* Instance(const char* configFileName = nullptr);
	CResourceBundleManage(const char* configFileName);
	virtual ~CResourceBundleManage(void);

private:
	std::unique_ptr<char[]> m_ResDir{};
	std::unique_ptr<char[]> m_ResLocale{};

	StringMap mapRes;
	ResourceBundle* m_pResourceBundle;
	UConverter *m_pConverter;
	int m_MaxSize;

	int m_LogFlag;

	FILE* m_LogFile;

private:
	UErrorCode ToCodePageString(UConverter *conv, UChar* source, char* target, int destCapacity, int& len);
	bool Init();

public:
	int GetSize(void);			// ȡ����Դ����
	
	void Release(void);

	const char* LoadResString(const char* key);
	UnicodeString LoadUResString(const char* key);

	int Format(const char* key, CFormatParameter& parameter, char buffer[]);
};
