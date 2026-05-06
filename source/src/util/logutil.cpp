//
// logutil.cpp
//
//  created by claude fan at 2004-8-9
//

#include "pch.h"
#include "log.h"
#include "log2.h"

#if 0
// 重定向printf
int myprintf(char const* fmt, ...)
    {
    char buf[8192];

    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    LG2("printf", buf);
    return 1;}

// 重定向fprintf
int myfprintf(FILE* fp, char const* fmt, ...)
    {
    char buf[8192];

    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    if (fp == stdout) {LG2("printf", buf);}
    else if (fp == stderr) {LG2("err", buf);}
    else LG2("other", buf);
    return 1;}

// 特点：要记录下每一个时间点上的LOG
static HWND LG_hwnd = nullptr;
void LG(char const* type, char const* format, ...)
    {
	// 检查是否使能LOG
	CLogMgr* logMgr = CLogMgr::Instance();
	if (logMgr == nullptr || !logMgr->IsEnable()) return;
	CLog* log = logMgr->Add(type);
	if (log == nullptr || !log->IsEnable()) return;

	char buf[8192] = {0};
	int len;
	va_list args;
	va_start(args, format);
	len = vsprintf(buf, format, args);
	va_end(args);
	if (len >= sizeof buf)
        throw std::logic_error("buf overflow, length > 8K\n");

    // 得到当前日期
    SYSTEMTIME st; char tim[100];
    ::GetLocalTime(&st);
    sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour,
            st.wMinute, st.wSecond);


	// total buffer
	char buffer[8192];
	


    //////////////////////////////////////////////////////////////////////////
	//
	// 写入磁盘文件的LOG
#if 0
    string ctx;
    if (strncmp(buf, "msg", 3) != 0)
        {
        ctx = "["; ctx += tim; ctx += "]"; ctx += buf;
        fLG(type, ctx.c_str());}
    else fLG(type, buf);
#endif

	if (strncmp(buf, "msg", 3) != 0)
		{
		len = sprintf(buffer, "[%s]%s", tim, buf);
		if (len >= sizeof buffer)
			throw std::logic_error("buffer overflow, length >= 2K\n");
		fLG(type, buffer);}
	else {
		fLG(type, buf);}


    //////////////////////////////////////////////////////////////////////////
    //
    // 发送到Logviewer的LOG
    if (LG_hwnd == nullptr)
        {LG_hwnd = FindWindow(nullptr, "kIng Of pIrAtEs lOgvIEwEr");}

    if (LG_hwnd == nullptr) return;

#if 0
    ctx.clear();
    ctx += "LOG!"; ctx += type;
    ctx += "!"; ctx += tim;
    ctx += "!"; ctx += buf;
    ctx += "!";
#endif

	buffer[0] = '\0';
	len = sprintf(buffer, "LOG!%s!%s!%s!", type, tim, buf);
	if (len >= sizeof buffer) throw std::logic_error("buffer overflow, length >= 2K\n");

    COPYDATASTRUCT cds;
    cds.dwData = sizeof cds;
//    cds.lpData = (PVOID)ctx.c_str();
//    cds.cbData = (DWORD)ctx.length() + 1;
    cds.lpData = (PVOID)buffer;
    cds.cbData = (DWORD)len + 1;
    if (FALSE == SendMessage(LG_hwnd, WM_COPYDATA,
                             reinterpret_cast<WPARAM>(LG_hwnd),
                             reinterpret_cast<LPARAM>(&cds)))
        {LG_hwnd = nullptr;}
    }
#endif

#if 1
// 特点：只记录下当前时间点上的LOG（更新）
static HWND GPL_hwnd = nullptr;
void GPL(char const* type, int x, int y, char const* format, ...) {

#if 0
    //////////////////////////////////////////////////////////////////////////
    //
    // 发送到Logviewer的GPL
    if (GPL_hwnd == nullptr)
        {GPL_hwnd = FindWindow(nullptr, "kIng Of pIrAtEs lOgvIEwEr");}

    if (GPL_hwnd == nullptr) return;

    // 得到LOG内容
    char buf[1024] = {0};
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    char xy[20]; sprintf(xy, "xy=%d", x + y * GPL_CONSTANT);

    string ctx;
    ctx += "GPL!"; ctx += type;
    ctx += "!"; ctx += xy;
    ctx += "!"; ctx += buf;
    ctx += "!";

    COPYDATASTRUCT cds;
    cds.dwData = sizeof cds;
    cds.lpData = (PVOID)ctx.c_str();
    cds.cbData = (DWORD)ctx.length() + 1;
    if (FALSE == SendMessage(GPL_hwnd, WM_COPYDATA,
                             reinterpret_cast<WPARAM>(GPL_hwnd),
                             reinterpret_cast<LPARAM>(&cds)))
        {GPL_hwnd = nullptr;}
#endif
}
#endif

#if 0

// 使能所有的LG输出
void EnableAllLG(bool enable /* = true */)
{
    CLogMgr::Instance()->Enable(enable);
}

// 使能特定的LG输出
void EnableLG(char const* type, bool enable /* = true */)
{
    CLog* pLog = nullptr;
    if (pLog = CLogMgr::Instance()->Add(type), pLog != nullptr)
	{
	    pLog->Enable(enable);
	}	
}
#endif

// new interface
void LG_EnableAll(bool bEnable /* = true */) {
	CLog2Mgr::Inst()->Enable(bEnable);
}
void LG_EnableMsgBox(bool bEnable /* = true */) {
	CLog2Mgr::Inst()->EnableMsgBox(bEnable);
}
void LG_SetDir(char const* szDir) {
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();

	pLogMgr->SetDirectory(szDir);
	pLogMgr->LogMgrReopen();
}
void LG_SetDirWithTimestamp(char const* szBaseDir) {
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();

	pLogMgr->SetDirectoryWithTimestamp(szBaseDir);
	pLogMgr->LogMgrReopen();
}
void LG_GetDir(std::string& strDir) {
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();

	pLogMgr->GetDirectory(strDir);
}
void LG_CloseAll() {
	CLog2Mgr::Inst()->CloseAll();
}
void LG_SetEraseMode(bool bEraseMode /* = true */) {
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();

	pLogMgr->bEraseMode = bEraseMode;
}
void LG2(char const* type, char const* format, ...) {
	// 检查是否使能LOG
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();
	if (pLogMgr == nullptr || !pLogMgr->IsEnable())
		return;

	CLog2* pLog = pLogMgr->Add(type);
	if (pLog == nullptr || !pLog->IsEnable())
		return;

	char buf[8192] = {0};
	int len;
	va_list args;
	va_start(args, format);
	len = vsprintf(buf, format, args);
	va_end(args);
	if (len >= sizeof buf) {
		pLogMgr->Log("stack buffer overflow when LG [%s]\n", type);
		return;
	}

	pLog->Log(buf);
}

void LG3(char const* type, char const* format, ...) {
	// 检查是否使能LOG
	CLog2Mgr* pLogMgr = CLog2Mgr::Inst();
	if (pLogMgr == nullptr || !pLogMgr->IsEnable())
		return;

	CLog2* pLog = pLogMgr->Add(type);
	if (pLog == nullptr || !pLog->IsEnable())
		return;

	char buf[8192] = {0};
	int len;
	va_list args;
	va_start(args, format);
	len = vsprintf(buf, format, args);
	va_end(args);
	if (len >= sizeof buf) {
		pLogMgr->Log("stack buffer overflow when LG [%s]\n", type);
		return;
	}

	pLog->Log3(buf);
}
