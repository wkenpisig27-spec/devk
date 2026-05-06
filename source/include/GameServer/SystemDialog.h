#ifdef PKO_PLATFORM_WINDOWS
#define WIN32_DLG
#endif

#ifdef WIN32_DLG

extern void CreateMainDialog(HINSTANCE hInst, HWND hParent);
extern void SystemReport(DWORD dwTime);
extern void MapReport();
extern HWND g_SysDlg;

#define WM_USER_MAP WM_USER + 0x100
#define WM_USER_LOG WM_USER + 0x101

#else

// Linux: No dialog — provide stub functions
inline void CreateMainDialog(void*, void*) {}
inline void SystemReport(unsigned int) {}
inline void MapReport() {}

#endif