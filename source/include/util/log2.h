
#ifndef LOG2_H_
#define LOG2_H_

#include <string>
#include <map>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#endif

//=============================================================================
// Log Levels - Can be used to filter log output
//=============================================================================
enum ELogLevel {
	LOG_LVL_NONE  = 0,    // No logging
	LOG_LVL_FATAL = 1,    // Fatal errors - application cannot continue
	LOG_LVL_ERROR = 2,    // Errors - something went wrong
	LOG_LVL_WARN  = 3,    // Warnings - something unexpected
	LOG_LVL_INFO  = 4,    // Info - general information (default)
	LOG_LVL_DEBUG = 5,    // Debug - detailed debugging info
	LOG_LVL_TRACE = 6     // Trace - very detailed tracing
};

class CThrdLock {
public:
	CThrdLock();
	~CThrdLock();

	void Lock();
	void Unlock();

private:
	CRITICAL_SECTION _cs;
};

#define LOGBUF_SIZE (8 * 1024)

class CLog2 {
public:
	CLog2();
	CLog2(char const* szName, char const* szExt = "log");
	~CLog2();

	bool Open(char const* szName, char const* szExt = "log");
	void Close();
	bool ReOpen();
	bool IsOpen() const { return _fp != nullptr; }  // Check if log file is open

	void Enable(bool bEnable = true);
	bool IsEnable() const;
	void Log(char const* szFormat, ...);
	void Log3(char const* szFormat, ...);
	
	// New: Log with level
	void LogLevel(ELogLevel level, char const* szFormat, ...);

	void SetMaxSize(DWORD dwSize);
	
	// New: Set minimum log level for this log
	void SetLogLevel(ELogLevel level) { _minLevel = level; }
	ELogLevel GetLogLevel() const { return _minLevel; }

protected:
	void _Init();
	bool _Open();
	bool _OpenX();

private:
	bool _bEnable;
	ELogLevel _minLevel;  // New: minimum level to log

	std::string _strName;
	std::string _strExt;
	int _dwMaxSize;
	int _nType;
	DWORD _dwLastFlushTick;  // Last time we flushed to disk

	FILE* _fp;
	CThrdLock _fpLock;
};
inline void CLog2::Enable(bool bEnable /* = true */) {
	_bEnable = bEnable;
}
inline bool CLog2::IsEnable() const {
	return _bEnable;
}
inline bool CLog2::ReOpen() {
	Close();
	return _Open();
}

class CLog2Mgr {
private:
	CLog2Mgr();
	~CLog2Mgr() {}
	CLog2Mgr(CLog2Mgr const&) {}
	CLog2Mgr& operator=(CLog2Mgr const&) {}

public:
	static CLog2Mgr* Inst();

	static void Enable(bool bEnable = true);
	static bool IsEnable();
	static void EnableMsgBox(bool bEnable = true);
	static bool IsEnableMsgBox();
	static void EnableConsole(bool bEnable = true);  // New: console output
	static bool IsEnableConsole();                    // New
	static void SetGlobalLogLevel(ELogLevel level);   // New: global min level
	static ELogLevel GetGlobalLogLevel();             // New
	static std::string* pstrLogDir;
	static bool bEraseMode;

#ifdef WIN32
	static void SetWnd(HWND hWnd);
	static HWND GetWnd();
#endif

	CLog2* Add(char const* szName, char const* szExt = "log");
	bool Remove(char const* szName);
	void RemoveAll();
	void CloseAll();

	void SetDirectory(char const* szDir);
	void SetDirectoryWithTimestamp(char const* szBaseDir);  // New: creates timestamped subdirectory
	void GetDirectory(std::string& strDir);

	void Log(char const* szFormat, ...);
	bool LogMgrReopen();

#if 0
    CLogMgr();

    char    m_szDir[255];


    CLog*   Get(const char *pszName);
    void    ClearAll();

    void        Log(FILE *fp, const char *pszLog, bool bMsg, bool bConsole); 
    void        SetCallBack(LOG_PROC pfn);
    void        RunCallBack(const char *pszType, const char *pszContent);
#endif

private:
	static CLog2Mgr* _pSelf;
	static bool _bEnable;
	static bool _bEnableMsgBox;
	static bool _bEnableConsole;       // New: console output flag
	static ELogLevel _globalMinLevel;  // New: global minimum log level
#ifdef WIN32
	static HWND _hWnd;
#endif

	std::map<std::string, CLog2*> _LogMap;
	CThrdLock _MapLock;

	CLog2 _LogMgr;
};
inline void CLog2Mgr::Enable(bool bEnable /* = true */) {
	_bEnable = bEnable;
}
inline bool CLog2Mgr::IsEnable() {
	return _bEnable;
}
inline void CLog2Mgr::EnableMsgBox(bool bEnable /* = true */) {
	_bEnableMsgBox = bEnable;
}
inline bool CLog2Mgr::IsEnableMsgBox() {
	return _bEnableMsgBox;
}
inline void CLog2Mgr::EnableConsole(bool bEnable /* = true */) {
	_bEnableConsole = bEnable;
}
inline bool CLog2Mgr::IsEnableConsole() {
	return _bEnableConsole;
}
inline void CLog2Mgr::SetGlobalLogLevel(ELogLevel level) {
	_globalMinLevel = level;
}
inline ELogLevel CLog2Mgr::GetGlobalLogLevel() {
	return _globalMinLevel;
}
inline bool CLog2Mgr::LogMgrReopen() {
	return _LogMgr.ReOpen();
}

#ifdef WIN32
inline void CLog2Mgr::SetWnd(HWND hWnd) {
	_hWnd = hWnd;
}
inline HWND CLog2Mgr::GetWnd() {
	return _hWnd;
}
#endif

//=============================================================================
// Convenience functions for level-based logging
//=============================================================================
void LG_Level(ELogLevel level, char const* type, char const* format, ...);

// Helper macros for level-based logging
#define LG_FATAL(type, fmt, ...) LG_Level(LOG_LVL_FATAL, type, fmt, ##__VA_ARGS__)
#define LG_ERROR(type, fmt, ...) LG_Level(LOG_LVL_ERROR, type, fmt, ##__VA_ARGS__)
#define LG_WARN(type, fmt, ...)  LG_Level(LOG_LVL_WARN, type, fmt, ##__VA_ARGS__)
#define LG_INFO(type, fmt, ...)  LG_Level(LOG_LVL_INFO, type, fmt, ##__VA_ARGS__)
#define LG_DEBUG(type, fmt, ...) LG_Level(LOG_LVL_DEBUG, type, fmt, ##__VA_ARGS__)
#define LG_TRACE(type, fmt, ...) LG_Level(LOG_LVL_TRACE, type, fmt, ##__VA_ARGS__)

// Get level name string
inline const char* GetLogLevelName(ELogLevel level) {
	switch (level) {
		case LOG_LVL_FATAL: return "FATAL";
		case LOG_LVL_ERROR: return "ERROR";
		case LOG_LVL_WARN:  return "WARN";
		case LOG_LVL_INFO:  return "INFO";
		case LOG_LVL_DEBUG: return "DEBUG";
		case LOG_LVL_TRACE: return "TRACE";
		default:            return "?????";
	}
}

#endif // LOG2_H_
