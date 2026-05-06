#ifndef __CATCH_STACK
#define __CATCH_STACK

#undef T_B
#undef T_E

#define __CATCH

#ifdef __CATCH

#define CATCH_EXCEPTION

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif
#include <exception>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#include "Stacktrace.h"
#else
// On Linux, platform_compat.h provides SYSTEMTIME/GetLocalTime used in macros below
#include "serversdk/platform_compat.h"

// Linux stub for SEHTranslator (Windows SEH not available on Linux)
class SEHTranslator {
public:
    SEHTranslator() {}
    ~SEHTranslator() {}
};
#endif

#endif

#if defined(CATCH_EXCEPTION)
#define T_FINAL                                                                    \
	}                                                                              \
	catch (std::exception & e) {                                                   \
		std::cout << e.what() << std::endl;                                        \
                                                                                   \
		std::string strfile;                                                       \
		LG_GetDir(strfile);                                                        \
		strfile += "/exception.txt";                                              \
		FILE* fp = fopen(strfile.c_str(), "a+");                                   \
                                                                                   \
		if (fp) {                                                                  \
			SYSTEMTIME st;                                                         \
			char tim[100] = {0};                                                   \
			GetLocalTime(&st);                                                     \
			sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, \
					st.wMinute, st.wSecond);                                       \
                                                                                   \
			fwrite(tim, strlen(tim), 1, fp);                                       \
			fwrite(__FUNCTION__, sizeof(__FUNCTION__) - 1, 1, fp);                 \
			fwrite(e.what(), strlen(e.what()) - 1, 1, fp);                         \
			fclose(fp);                                                            \
		}                                                                          \
	}
#define T_REPORT                                                                   \
	}                                                                              \
	catch (std::exception & e) {                                                   \
		std::string strfile;                                                       \
		LG_GetDir(strfile);                                                        \
		strfile += "/exception.txt";                                               \
		FILE* fp = fopen(strfile.c_str(), "a+");                                   \
                                                                                   \
		if (fp) {                                                                  \
			SYSTEMTIME st;                                                         \
			char tim[100] = {0};                                                   \
			GetLocalTime(&st);                                                     \
			sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, \
					st.wMinute, st.wSecond);                                       \
                                                                                   \
			fwrite(tim, strlen(tim), 1, fp);                                       \
			fwrite(__FUNCTION__, sizeof(__FUNCTION__) - 1, 1, fp);                 \
			fwrite(e.what(), strlen(e.what()) - 1, 1, fp);                         \
			fclose(fp);                                                            \
		}                                                                          \
	}
#define T_B try {

#define T_E                                                                        \
	}                                                                              \
	catch (std::exception & e) {                                                   \
		std::string strfile;                                                       \
		LG_GetDir(strfile);                                                        \
		strfile += "/exception.txt";                                              \
		FILE* fp = fopen(strfile.c_str(), "a+");                                   \
                                                                                   \
		if (fp) {                                                                  \
			SYSTEMTIME st;                                                         \
			char tim[100] = {0};                                                   \
			GetLocalTime(&st);                                                     \
			sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, \
					st.wMinute, st.wSecond);                                       \
                                                                                   \
			fwrite(tim, strlen(tim), 1, fp);                                       \
			fwrite(__FUNCTION__, sizeof(__FUNCTION__) - 1, 1, fp);                 \
			fwrite(e.what(), strlen(e.what()) - 1, 1, fp);                         \
			fclose(fp);                                                            \
		}                                                                          \
		throw;                                                                     \
	}
#define T_EXIT                                                                         \
	}                                                                                  \
	catch (std::exception & e) {                                                       \
		try {                                                                          \
			std::string strfile;                                                       \
			LG_GetDir(strfile);                                                        \
			strfile += "/exception.txt";                                              \
			FILE* fp = fopen(strfile.c_str(), "a+");                                   \
                                                                                       \
			if (fp) {                                                                  \
				SYSTEMTIME st;                                                         \
				char tim[100] = {0};                                                   \
				GetLocalTime(&st);                                                     \
				sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, \
						st.wMinute, st.wSecond);                                       \
                                                                                       \
				fwrite(tim, strlen(tim), 1, fp);                                       \
				fwrite(__FUNCTION__, sizeof(__FUNCTION__) - 1, 1, fp);                 \
				fwrite(e.what(), strlen(e.what()) - 1, 1, fp);                         \
				fclose(fp);                                                            \
			}                                                                          \
			ChaException(g_ulCurID, g_lCurHandle);                                     \
		} catch (...) {                                                                \
		}                                                                              \
	}
#define TRY_END_INFO(str)                                                          \
	}                                                                              \
	catch (...) {                                                                  \
		std::string strfile;                                                       \
		LG_GetDir(strfile);                                                        \
		strfile += "/exception.txt";                                              \
		FILE* fp = fopen(strfile.c_str(), "a+");                                   \
                                                                                   \
		if (fp) {                                                                  \
			SYSTEMTIME st;                                                         \
			char tim[100] = {0};                                                   \
			GetLocalTime(&st);                                                     \
			sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, \
					st.wMinute, st.wSecond);                                       \
                                                                                   \
			fwrite(tim, strlen(tim), 1, fp);                                       \
			fwrite(__FUNCTION__, sizeof(__FUNCTION__) - 1, 1, fp);                 \
			fwrite(e.what(), strlen(e.what()) - 1, 1, fp);                         \
			fclose(fp);                                                            \
		}                                                                          \
		throw;                                                                     \
	}
#define TRY_END_NOTHROW \
	}                   \
	catch (...) {       \
	}
#define TRY_END_RETURN(str) \
	}                       \
	catch (...) {           \
		return str;         \
	}

#else

#define T_B
#define T_E
#define T_FINAL
#define TRY_END_INFO(str)
#define TRY_END_NOTHROW
#define TRY_END_RETURN(str)

#endif

#endif
