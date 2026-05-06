
#include "log2.h"
#include "util2.h"
#include <stdexcept>

CLog2::CLog2() : _fp(nullptr), _fpLock(), _strName(""), _strExt("") {
	_bEnable = true;
	_nType = 0;
	_minLevel = LOG_LVL_INFO;  // Default to INFO level
	_dwLastFlushTick = 0;
	SetMaxSize(100 * 1024 * 1024);
}
CLog2::CLog2(char const* szName, char const* szExt /* = "log" */)
	: _fp(nullptr), _fpLock(), _strName(szName), _strExt(szExt) {
	_bEnable = true;
	_nType = 0;
	_minLevel = LOG_LVL_INFO;  // Default to INFO level
	_dwLastFlushTick = 0;
	SetMaxSize(100 * 1024 * 1024);

	if (!_Open())
		throw std::logic_error("Open file failed\n");
}
CLog2::~CLog2() {
	Close();
	_nType = 0;
}
bool CLog2::Open(char const* szName, char const* szExt) {
	if (_fp != nullptr)
		return false;

	_strName = szName;
	_strExt = szExt;
	return _Open();
}
void CLog2::SetMaxSize(DWORD dwSize) {
	_dwMaxSize = dwSize;
}
bool CLog2::_Open() {
	if (_fp != nullptr)
		return false;

	std::string str = *CLog2Mgr::pstrLogDir;
	str += '/';
	str += _strName;
	if (_strExt.length() > 0) {
		str += ".";
		str += _strExt;
	}

	_fpLock.Lock();
	do {
		if (_fp != nullptr)
			break;

		try {
			if (CLog2Mgr::bEraseMode) {
				_fp = fopen(str.c_str(), "wt");
			} else {
				_fp = fopen(str.c_str(), "at+");

				if (_fp == nullptr) {
					_fp = fopen(str.c_str(), "wt");
				}
			}
		} catch (...) {
		}
	} while (0);
	_fpLock.Unlock();

	return (_fp == nullptr) ? false : true;
}

bool CLog2::_OpenX() {
	if (_fp != nullptr && _nType != 0)
		return false;

	std::string str = *CLog2Mgr::pstrLogDir;
	str += '/';
	str += _strName;
	if (_strExt.length() > 0) {
		str += ".";
		str += _strExt;
	}

	_fpLock.Lock();
	do {
		if (_fp != nullptr) {
			Close();
		}

		try {
			_fp = fopen(str.c_str(), "rt+");
			if (_fp == nullptr) {
				_fp = fopen(str.c_str(), "wt");
			}
			_nType = 1;
		} catch (...) {
		}
	} while (0);
	_fpLock.Unlock();

	return (_fp == nullptr) ? false : true;
}

void CLog2::Close() {
	if (_fp != nullptr) {
		_fpLock.Lock();
		try {
			fclose(_fp);
			_fp = nullptr;
			_nType = 0;
		} catch (...) {
		}
		_fpLock.Unlock();
	}
}
void CLog2::Log(char const* szFormat, ...) {
	_Open();

	bool bMsgBox = false;
	if ((strlen(szFormat) > 3) && (strncmp(szFormat, "msg", 3) == 0)) {
		bMsgBox = true;
	}

	char szBuf[LOGBUF_SIZE] = {0};
	va_list list;
	va_start(list, szFormat);
	if (bMsgBox) {
		vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat + 3, list);  // Safe: use vsnprintf
	} else {
		vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat, list);      // Safe: use vsnprintf
	}
	szBuf[LOGBUF_SIZE - 1] = '\0';  // Ensure null termination
	va_end(list);

	char buf[LOGBUF_SIZE] = {0};
	SYSTEMTIME st;
	char tim[100] = {0};
	GetLocalTime(&st);
	snprintf(tim, sizeof(tim) - 1, "%02d-%02d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wHour,
			st.wMinute, st.wSecond, st.wMilliseconds);  // Added milliseconds

	if (bMsgBox) {
		snprintf(buf, LOGBUF_SIZE - 1, "[%s][MSG]%s", tim, szBuf);  // Safe: use snprintf
	} else {
		snprintf(buf, LOGBUF_SIZE - 1, "[%s]%s", tim, szBuf);       // Safe: use snprintf
	}
	buf[LOGBUF_SIZE - 1] = '\0';

	// Log to file
	_fpLock.Lock();
	try {
		// Wrap file at max size to prevent unbounded growth
		if (_fp && ftell(_fp) >= _dwMaxSize) {
			fseek(_fp, 0, SEEK_SET);
		}

		if (_fp) {
			fprintf(_fp, "%s", buf);

			// Periodic flush every 5 seconds instead of every call
			// Avoids synchronous disk writes that cause game thread stalls
			DWORD dwNow = GetTickCount();
			if (dwNow - _dwLastFlushTick >= 5000) {
				fflush(_fp);
				_dwLastFlushTick = dwNow;
			}
		}
	} catch (...) {
	}
	_fpLock.Unlock();

	// Console output (new feature)
	if (CLog2Mgr::IsEnableConsole()) {
#ifdef WIN32
		OutputDebugStringA(buf);
#endif
		printf("%s", buf);
	}

	// MsgBox
#ifdef WIN32
	if (bMsgBox && CLog2Mgr::IsEnableMsgBox()) {
		MessageBox(CLog2Mgr::GetWnd(), szBuf, "LOG", 0);
	}
#endif
}

void CLog2::Log3(char const* szFormat, ...) {
	_OpenX();

	bool bMsgBox = false;
	if ((strlen(szFormat) > 3) && (strncmp(szFormat, "msg", 3) == 0)) {
		bMsgBox = true;
	}

	char szBuf[LOGBUF_SIZE] = {0};
	va_list list;
	va_start(list, szFormat);
	if (bMsgBox) {
		vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat + 3, list);  // Safe: use vsnprintf
	} else {
		vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat, list);      // Safe: use vsnprintf
	}
	szBuf[LOGBUF_SIZE - 1] = '\0';
	va_end(list);

	char buf[LOGBUF_SIZE] = {0};
	SYSTEMTIME st;
	char tim[100] = {0};
	GetLocalTime(&st);
	snprintf(tim, sizeof(tim) - 1, "%02d-%02d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wHour,
			st.wMinute, st.wSecond, st.wMilliseconds);

	if (bMsgBox) {
		snprintf(buf, LOGBUF_SIZE - 1, "[%s][MSG]%s", tim, szBuf);
	} else {
		snprintf(buf, LOGBUF_SIZE - 1, "[%s]%s", tim, szBuf);
	}
	buf[LOGBUF_SIZE - 1] = '\0';

	// Log
	_fpLock.Lock();
	try {
		if (_fp && ftell(_fp) >= _dwMaxSize) {
			fseek(_fp, 0, SEEK_SET);
		}

		if (_fp) {
			fprintf(_fp, "%s", buf);

			// Periodic flush every 5 seconds instead of every call
			DWORD dwNow = GetTickCount();
			if (dwNow - _dwLastFlushTick >= 5000) {
				fflush(_fp);
				_dwLastFlushTick = dwNow;
			}
		}
	} catch (...) {
	}
	_fpLock.Unlock();

	// MsgBox
#ifdef WIN32
	if (bMsgBox && CLog2Mgr::IsEnableMsgBox()) {
		MessageBox(CLog2Mgr::GetWnd(), szBuf, "LOG", 0);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//
//  CLog2Mgr class
//
//
CLog2Mgr* CLog2Mgr::_pSelf = nullptr;
bool CLog2Mgr::_bEnable;
bool CLog2Mgr::_bEnableMsgBox;
bool CLog2Mgr::_bEnableConsole = false;       // New: console output disabled by default
ELogLevel CLog2Mgr::_globalMinLevel = LOG_LVL_INFO;  // New: default to INFO level
std::string* CLog2Mgr::pstrLogDir = nullptr;
bool CLog2Mgr::bEraseMode = false;
#ifdef WIN32
HWND CLog2Mgr::_hWnd;
#endif

CLog2Mgr::CLog2Mgr() : _MapLock(), _LogMap() {
	_setmaxstdio(2048);

	_bEnable = true;
	_bEnableMsgBox = true;
	_bEnableConsole = false;
	_globalMinLevel = LOG_LVL_INFO;
	pstrLogDir = new std::string("log");

#ifdef WIN32
	_hWnd = nullptr;
#endif

	// Don't set directory or open LGMgr here - defer until SetDirectoryWithTimestamp is called
	// This prevents logs being created in wrong location before server configures its log dir
}
CLog2Mgr* CLog2Mgr::Inst() {
	static bool bInit = false;

	if (!bInit) {
		static CThrdLock Lock;
		Lock.Lock();
		if (!bInit) {
			try {
				CLog2Mgr::_pSelf = new CLog2Mgr();
				bInit = true;
			} catch (std::bad_alloc&) {
#ifdef WIN32
				OutputDebugString("Failed to new CLog2Mgr\n");
#endif
			} catch (std::logic_error& le) {
#ifdef WIN32
				OutputDebugString(le.what());
#endif
				delete CLog2Mgr::_pSelf;
				CLog2Mgr::_pSelf = nullptr;
			} catch (...) {
#ifdef WIN32
				OutputDebugString("Unknown exception raised from CLog2Mgr::Inst()\n");
#endif
				delete CLog2Mgr::_pSelf;
				CLog2Mgr::_pSelf = nullptr;
			}
		}
		Lock.Unlock();
	}

	return CLog2Mgr::_pSelf;
}
CLog2* CLog2Mgr::Add(char const* szName, char const* szExt /* = "log" */) {
	CLog2* pLog = nullptr;
	_MapLock.Lock();
	try {
		std::map<std::string, CLog2*>::iterator it = _LogMap.find(szName);
		if (it != _LogMap.end()) {
			pLog = (*it).second;
		} else {
			try {
				pLog = new CLog2(szName, szExt);
			} catch (std::bad_alloc&) {
				_LogMgr.Log("Failed to new CLog2([%s.%s])\n", szName, szExt);
			} catch (std::logic_error& le) {
				_LogMgr.Log(le.what());
				delete pLog;
				pLog = nullptr;
			} catch (...) {
				_LogMgr.Log("Unknown exception raised from new [%s.%s]\n", szName, szExt);
				delete pLog;
				pLog = nullptr;
			}

			if (pLog != nullptr) {
				_LogMap[szName] = pLog;
				_LogMgr.Log("Add LG [%s]\n", szName);
			}
		}
	} catch (...) {
	}
	_MapLock.Unlock();

	return pLog;
}
bool CLog2Mgr::Remove(char const* szName) {
	_LogMgr.Log("Remove LG [%s]\n", szName);

	bool ret;
	_MapLock.Lock();
	try {
		std::map<std::string, CLog2*>::iterator it = _LogMap.find(szName);
		if (it != _LogMap.end()) {
			CLog2* pLog = (*it).second;
			_LogMap.erase(it);
			delete pLog;
			ret = true;
		} else {
			ret = false;
		}
	} catch (...) {
		_LogMgr.Log("Unknown excepton raised from CLog2Mgr::Remove([%s])\n", szName);
		ret = false;
	}
	_MapLock.Unlock();

	return ret;
}
void CLog2Mgr::RemoveAll() {
	_LogMgr.Log("Remove All LG\n");

	CLog2* pLog = nullptr;
	_MapLock.Lock();
	try {
		std::map<std::string, CLog2*>::iterator it = _LogMap.begin();
		while (it != _LogMap.end()) {
			pLog = (*it).second;
			_LogMap.erase(it);
			delete pLog;

			++it;
		}
	} catch (...) {
		_LogMgr.Log("Unknown excepton raised from CLog2Mgr::RemoveAll()\n");
	}
	_MapLock.Unlock();
}
void CLog2Mgr::CloseAll() {
	_LogMgr.Log("Close All LG\n");

	CLog2* pLog = nullptr;
	_MapLock.Lock();
	try {
		DWORD dwBegin = ::GetTickCount();
		DWORD dwClose = 0;

		std::map<std::string, CLog2*>::iterator it = _LogMap.begin();
		while (it != _LogMap.end()) {
			pLog = (*it).second;

			DWORD dwBegin = ::GetTickCount();
			pLog->Close();
			DWORD dwEnd = ::GetTickCount();
			dwClose += dwEnd - dwBegin;

			++it;
		}

		DWORD dwEnd = ::GetTickCount();
		_LogMgr.Log("CloseAll consume %ds\n", (dwEnd - dwBegin) / 1000);
		_LogMgr.Log("Close consume %ds\n", dwClose / 1000);
	} catch (...) {
		_LogMgr.Log("Unknown excepton raised from CLog2Mgr::CloseAll()\n");
	}
	_MapLock.Unlock();
}
void CLog2Mgr::SetDirectory(char const* szDir) {
	*pstrLogDir = szDir;
	Util_MakeDir(szDir);
}
void CLog2Mgr::SetDirectoryWithTimestamp(char const* szBaseDir) {
	// Create base directory first
	Util_MakeDir(szBaseDir);
	
	// Get current date
	SYSTEMTIME st;
	GetLocalTime(&st);
	
	// Format: BaseDir/YYYY-MM-DD (one folder per day)
	char szDateDir[512];
	snprintf(szDateDir, sizeof(szDateDir), 
		"%s/%04d-%02d-%02d",
		szBaseDir, st.wYear, st.wMonth, st.wDay);
	
	*pstrLogDir = szDateDir;
	Util_MakeDir(szDateDir);
	
	// Now that directory is properly set, open the manager log
	if (!_LogMgr.IsOpen()) {
		_LogMgr.Open("LGMgr");
	}
}
void CLog2Mgr::GetDirectory(std::string& strDir) {
	strDir = *pstrLogDir;
}
void CLog2Mgr::Log(char const* szFormat, ...) {
	char szBuf[LOGBUF_SIZE] = {0};

	va_list list;
	va_start(list, szFormat);
	vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat, list);  // Safe: use vsnprintf
	szBuf[LOGBUF_SIZE - 1] = '\0';
	va_end(list);

	_LogMgr.Log(szBuf);
}

//////////////////////////////////////////////////////////////////////////
//
//  CThrdLock class
//
//
CThrdLock::CThrdLock() {
	InitializeCriticalSection(&_cs);
}
CThrdLock::~CThrdLock() {
	DeleteCriticalSection(&_cs);
}
void CThrdLock::Lock() {
	EnterCriticalSection(&_cs);
}
void CThrdLock::Unlock() {
	LeaveCriticalSection(&_cs);
}

//////////////////////////////////////////////////////////////////////////
//
//  CLog2::LogLevel - Log with level filtering
//
//
void CLog2::LogLevel(ELogLevel level, char const* szFormat, ...) {
	// Check if this level should be logged
	if (level > _minLevel || level > CLog2Mgr::GetGlobalLogLevel()) {
		return;  // Skip if below minimum level
	}

	_Open();

	char szBuf[LOGBUF_SIZE] = {0};
	va_list list;
	va_start(list, szFormat);
	vsnprintf(szBuf, LOGBUF_SIZE - 1, szFormat, list);
	szBuf[LOGBUF_SIZE - 1] = '\0';
	va_end(list);

	char buf[LOGBUF_SIZE] = {0};
	SYSTEMTIME st;
	char tim[100] = {0};
	GetLocalTime(&st);
	snprintf(tim, sizeof(tim) - 1, "%02d-%02d %02d:%02d:%02d.%03d", 
			st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	// Include level prefix
	snprintf(buf, LOGBUF_SIZE - 1, "[%s][%s]%s", tim, GetLogLevelName(level), szBuf);
	buf[LOGBUF_SIZE - 1] = '\0';

	// Log to file
	_fpLock.Lock();
	try {
		if (_fp) {
			fprintf(_fp, "%s", buf);
			fflush(_fp);
		}
	} catch (...) {
	}
	_fpLock.Unlock();

	// Console output
	if (CLog2Mgr::IsEnableConsole()) {
#ifdef WIN32
		OutputDebugStringA(buf);
#endif
		printf("%s", buf);
	}
}

//////////////////////////////////////////////////////////////////////////
//
//  LG_Level - Global function for level-based logging
//
//
void LG_Level(ELogLevel level, char const* type, char const* format, ...) {
	// Check global level filter
	if (level > CLog2Mgr::GetGlobalLogLevel()) {
		return;
	}

	CLog2* pLog = CLog2Mgr::Inst()->Add(type);
	if (!pLog || !pLog->IsEnable()) {
		return;
	}

	char szBuf[LOGBUF_SIZE] = {0};
	va_list list;
	va_start(list, format);
	vsnprintf(szBuf, LOGBUF_SIZE - 1, format, list);
	szBuf[LOGBUF_SIZE - 1] = '\0';
	va_end(list);

	pLog->LogLevel(level, "%s", szBuf);
}
