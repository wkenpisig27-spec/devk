#pragma once

#include "lock.h"
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <string>

#define SAFE_DELETE(p) \
	if (p) {           \
		delete p;      \
		p = nullptr;      \
	}

#define SAFE_DELETE_ARRAY(p) \
	if (p) {                 \
		delete[] p;          \
		p = nullptr;            \
	}

#define __BEGIN_TRY try {
#define __END_TRY                  \
	}                              \
	catch (...) {                  \
	}

class Log {
public:
	Log();
	~Log();

	void Open(const char* file);
	void Close();
	void WriteLog(const char* buf);

private:
	FILE* m_file;
	Lock m_lock;
};

typedef std::map<std::string, Log*> LogList;

class LogMgr {
public:
	LogMgr();
	~LogMgr();

	Log* GetFileHandle(const char* filename);
	void CloseLogFile(const char* filename);
	void CloseAllLogFile();

private:
	LogList m_list;
	Lock m_lock;
	char m_path[MAX_PATH];
};

extern LogMgr g_lm;

void LG(const char* filename, const char* format, ...);
void LGFMT(const char* filename, const char* format, va_list param);
void LGPRT(const char* filename, const char* format, ...);
void CloseAllLog();
