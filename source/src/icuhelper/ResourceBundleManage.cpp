#include "ResourceBundleManage.h"

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#else
#include "serversdk/platform_compat.h"
#endif
#include <string>
#include <memory>

using namespace std;
using namespace icu;

#include "pi_Alloc.h"

#define BUFFER_SIZE 255

class CLogLock {
public:
	CLogLock() { InitializeCriticalSection(&_cs); }
	~CLogLock() { DeleteCriticalSection(&_cs); }

	void Lock() { EnterCriticalSection(&_cs); }
	void Unlock() { LeaveCriticalSection(&_cs); }

private:
	CRITICAL_SECTION _cs;
};

CFormatParameter::CFormatParameter(int paraNum) {
	UErrorCode status = U_ZERO_ERROR;
	m_MsgArgs = new Formattable[paraNum];
	this->paraNum = paraNum;
}

CFormatParameter::~CFormatParameter() {
	if (m_MsgArgs) {
		delete[] m_MsgArgs;
		m_MsgArgs = nullptr;
	}
}

void CFormatParameter::setDouble(int index, double d) {
	m_MsgArgs[index].setDouble(d);
}

void CFormatParameter::setLong(int index, int32_t l) {
	m_MsgArgs[index].setLong(l);
}

void CFormatParameter::setInt64(int index, int64_t ll) {
	m_MsgArgs[index].setInt64(ll);
}

void CFormatParameter::setDate(int index, UDate d) {
	m_MsgArgs[index].setDate(d);
}

void CFormatParameter::setString(int index, const UnicodeString& stringToCopy) {
	m_MsgArgs[index].setString(stringToCopy);
}

CResourceBundleManage* CResourceBundleManage::_instance = nullptr;

CResourceBundleManage* CResourceBundleManage::Instance(const char* configFileName) {
	if (_instance == nullptr && configFileName == nullptr) {
		throw "Resource is not instance!";
	}

	if (_instance != nullptr && configFileName != nullptr) {
		throw "Resource is already instance!";
		//_instance = new CResourceBundleManage();
	}

	if (_instance == nullptr) {
		_instance = new CResourceBundleManage(configFileName);
	}

	return _instance;
}

CResourceBundleManage::CResourceBundleManage(const char* configFileName)
	: m_pResourceBundle(nullptr), m_pConverter(nullptr), m_MaxSize(0), m_LogFile(nullptr), m_LogFlag(0) {
	
	if (_instance == nullptr) {
		_instance = this;
	}

	char szPath[MAX_PATH];

	if (!GetModuleFileName(nullptr, szPath, MAX_PATH)) {
		throw "GetModuleFileName failed \n";
	}
	// Get the directory of the executable
	const char* ret = strrchr(szPath, '/');
	if (!ret) ret = strrchr(szPath, '\\');
	if (!ret) {
		throw "Cannot determine executable directory";
	}
	int dirLength = (int)(ret - szPath);

	size_t length = strlen(configFileName);
	// char* fileName = new char[dirLength + length + 2 ];
	auto fileName = std::make_unique<char[]>(dirLength + length + 2);
	memcpy(fileName.get(), szPath, dirLength);
#ifdef _WIN32
	fileName[dirLength] = '\\';
#else
	fileName[dirLength] = '/';
#endif
	memcpy(fileName.get() + dirLength + 1, configFileName, length);
	fileName[dirLength + length + 1] = 0;

	UErrorCode status = U_ZERO_ERROR;
	Locale::setDefault(Locale::getEnglish(), status);

	const char* lpAppName = "locale";
	const char* lpLocaleKeyName = "locale";
	const char* lpPathKeyName = "path";
	const char* lpDefaultLocale = "en_US";
	// char* lpDefaultPath = new char[dirLength + 1];
	auto lpDefaultPath = std::make_unique<char[]>(dirLength + 1);
	memcpy(lpDefaultPath.get(), szPath, dirLength);
	lpDefaultPath[dirLength] = 0;

	char lpReturnedLocaleString[BUFFER_SIZE];
	char lpReturnedPathString[BUFFER_SIZE];

	length = GetPrivateProfileStringA(lpAppName, lpLocaleKeyName, lpDefaultLocale, lpReturnedLocaleString, sizeof(lpReturnedLocaleString), fileName.get());

	m_ResLocale = std::make_unique<char[]>(length + 1);
	memcpy(m_ResLocale.get(), lpReturnedLocaleString, length);
	m_ResLocale[length] = 0;

	length = GetPrivateProfileStringA(lpAppName, lpPathKeyName, lpDefaultPath.get(), lpReturnedPathString, sizeof(lpReturnedPathString), fileName.get());

	// If path is "." or empty, use the executable directory
	if (length == 0 || (length == 1 && lpReturnedPathString[0] == '.')) {
		memcpy(lpReturnedPathString, lpDefaultPath.get(), dirLength);
		lpReturnedPathString[dirLength] = 0;
		length = dirLength;
	}

	m_ResDir = make_unique<char[]>(length + 1);
	memcpy(m_ResDir.get(), lpReturnedPathString, length);
	m_ResDir[length] = 0;

	m_LogFlag = GetPrivateProfileInt(lpAppName, "log", 0, fileName.get());

	if (m_LogFlag) {
#ifdef _WIN32
		string logFileName = string(m_ResDir.get()) + "\\res.log";
#else
		string logFileName = string(m_ResDir.get()) + "/res.log";
#endif
		m_LogFile = fopen(logFileName.c_str(), "w+");
	}

	status = U_ZERO_ERROR;
	m_pConverter = ucnv_open(nullptr, &status); // ucnv_open("cp936", &status); // windows-936-2000

	if (status == U_ZERO_ERROR) {

	} else if (status == U_MEMORY_ALLOCATION_ERROR)
		::MessageBox(nullptr, "Memory allocation error", nullptr, 0);
	else if (status == U_FILE_ACCESS_ERROR)
		::MessageBox(nullptr, "The requested file cannot be found", nullptr, 0);
	else
		::MessageBox(nullptr, "Other error!", nullptr, 0);

	m_MaxSize = ucnv_getMaxCharSize(m_pConverter);

	status = U_ZERO_ERROR;
	m_pResourceBundle = new ResourceBundle(m_ResDir.get(), m_ResLocale.get(), status);

	if (U_FAILURE(status)) {
		char errorMsg[512];
		sprintf(errorMsg, "Failed to load resource bundle!\nPath: %s\nLocale: %s\nError: %s", 
			m_ResDir.get(), m_ResLocale.get(), u_errorName(status));
		::MessageBox(nullptr, errorMsg, "ResourceBundle Error", MB_OK | MB_ICONERROR);
		// Clean up on failure
		delete m_pResourceBundle;
		m_pResourceBundle = nullptr;
	}

	Init();
}

CResourceBundleManage::~CResourceBundleManage(void) {
	Release();
}

// 取得资源个数
int CResourceBundleManage::GetSize(void) {
	return (int)mapRes.size();
}

// 把Unicode字符串转换为多字节本机编码
UErrorCode CResourceBundleManage::ToCodePageString(UConverter* conv, UChar* source, char* target, int destCapacity, int& len) {
	UErrorCode status = U_ZERO_ERROR;

	len = ucnv_fromUChars(conv, target, destCapacity, source, -1, &status);

	if (U_SUCCESS(status) == FALSE)
		return status;

	return status;
}

// 初始化资源管理器
bool CResourceBundleManage::Init() {
	int len = 0;
	int maxSize = 0;

	UErrorCode status = U_ZERO_ERROR;

#if _DEBUG

	const char* name = ucnv_getName(m_pConverter, &status);
	printf("Current CodePage is %s\n", name);

#endif

	status = U_ZERO_ERROR;

	// Check if the resource bundle was loaded successfully
	if (m_pResourceBundle == nullptr) {
		return false;
	}

	m_pResourceBundle->resetIterator();

	while (m_pResourceBundle->hasNext()) {
		ResourceBundle bundle = m_pResourceBundle->getNext(status);

		if (U_FAILURE(status)) {
			break;
		}

		const char* key = bundle.getKey();

		UnicodeString value = bundle.getString(status);

		if (U_FAILURE(status)) {
			continue;
		}

		len = 0;
		maxSize = m_MaxSize * (1 + value.length());

		auto buffer = std::make_unique<char[]>(maxSize);
		memset(buffer.get(), 0, maxSize);

		ToCodePageString(m_pConverter, const_cast<UChar*>(value.getTerminatedBuffer()), buffer.get(), maxSize, len);

		// Use std::string key to avoid dangling pointer
		mapRes[std::string(key)] = std::move(buffer);
	}

	return mapRes.size() > 0;
}

// 释放资源
void CResourceBundleManage::Release(void) {
	m_ResDir.reset(nullptr);
	m_ResLocale.reset(nullptr);
	mapRes.clear();

	if (m_pResourceBundle != nullptr) {
		delete m_pResourceBundle;
		m_pResourceBundle = nullptr;
	}

	if (m_pConverter != nullptr) {
		ucnv_close(m_pConverter);
		m_pConverter = nullptr;
	}

	if (m_LogFlag) {
		if (m_LogFile) {
			fclose(m_LogFile);
			m_LogFile = nullptr;
		}
	}
}

// 根据ID取得字符串
const char* CResourceBundleManage::LoadResString(const char* key) {
	const char* ret = "";
	StringMap::iterator in = mapRes.find(key);

	if (in != mapRes.end()) {
		ret = in->second.get();
	}
#if _DEBUG
	// printf("key = %s value = %s\n",key, ret);
#endif

	static CLogLock lock;
	lock.Lock();
	if (m_LogFlag) {
		if (m_LogFile) {
			fprintf(m_LogFile, "Key = [%s] Value = %s\r\n", key, ret);
		}
	}
	lock.Unlock();

	return ret;
}

UnicodeString CResourceBundleManage::LoadUResString(const char* key) {
	UErrorCode status = U_ZERO_ERROR;
	return m_pResourceBundle->getStringEx(key, status);
}

// 格式根据参数化一个字符串
int CResourceBundleManage::Format(const char* key, CFormatParameter& parameter, char buffer[]) {
	UErrorCode status = U_ZERO_ERROR;
	UnicodeString str;
	FieldPosition pos;

	UnicodeString format = LoadUResString(key);

	// Create a message format
	MessageFormat msg(format, status);
	msg.format(parameter.GetMsgArgs(), parameter.GetParaNum(), str, pos, status);

	int aaa = m_MaxSize;

	int len = 0;
	int maxSize = m_MaxSize * (1 + str.length());

	ToCodePageString(m_pConverter, const_cast<UChar*>(str.getTerminatedBuffer()), buffer, maxSize, len);

	return len;
}