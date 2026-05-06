#include "pi_Memory.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#endif

using namespace std;

pi_Memory::pi_Memory(const char* fileName) : m_bStop(false) {
	m_szfileName = fileName;
}

pi_Memory::~pi_Memory(void) {
}

bool pi_Memory::startMonitor(int minuteInterval, DWORD processID) {
	this->m_minuteInterval = minuteInterval;
	this->m_processID = processID;

	m_outputFile.open(m_szfileName.c_str(), ios::out);
	m_outputFile << "time,memory size,virtual memory size,physical available size" << endl;

	m_hThread = CreateThread(nullptr, 0, ThreadProc, this, 0, &m_dwThreadId);
	return true;
}

bool pi_Memory::stopMonitor() {
	m_bStop = true;

	return true;
}

bool pi_Memory::wait() {
	DWORD exitCode;

	if (GetExitCodeThread(m_hThread, &exitCode)) {
		if (exitCode == STILL_ACTIVE) {
			ResumeThread(m_hThread);
			if (TerminateThread(m_hThread, 1)) {
				printf("TerminateThread OK!");
			} else {
				DWORD error = GetLastError();
				printf("TerminateThread Error(%d)!", error);
			}
		}
	}

	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);

	if (m_outputFile.is_open())
		m_outputFile.close();

	return true;
}

DWORD WINAPI pi_Memory::ThreadProc(LPVOID lpParam) {
	pi_Memory* pThis = (pi_Memory*)lpParam;

	while (!pThis->m_bStop) {
#if defined(_WIN32) || defined(_WIN64)
		HANDLE hProcess;
		PERFORMANCE_INFORMATION pi;
		PROCESS_MEMORY_COUNTERS pmc;

		try {
			// Print information about the memory usage of the process.
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pThis->m_processID);
			if (nullptr == hProcess)
				continue;

			if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)) && GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION))) {
				SYSTEMTIME st;
				char time[15] = {0};
				GetLocalTime(&st);
				sprintf(time, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

				pThis->m_outputFile << time << "," << pmc.WorkingSetSize / (1024.0 * 1024.0) << "," << pmc.PagefileUsage / (1024.0 * 1024.0) << ","
									<< pi.PhysicalAvailable * pi.PageSize / (1024.0 * 1024.0) << endl;
			}

			CloseHandle(hProcess);
		} catch (...) {
		}
#else
		// Linux: read memory info from /proc
		try {
			double workingSetMB = 0.0, virtualMB = 0.0, physAvailMB = 0.0;
			// Read process memory from /proc/self/status
			std::ifstream procStatus("/proc/self/status");
			std::string line;
			while (std::getline(procStatus, line)) {
				if (line.compare(0, 6, "VmRSS:") == 0)
					workingSetMB = std::strtod(line.c_str() + 6, nullptr) / 1024.0; // kB -> MB
				else if (line.compare(0, 7, "VmSize:") == 0)
					virtualMB = std::strtod(line.c_str() + 7, nullptr) / 1024.0;
			}
			// Read available memory from /proc/meminfo
			std::ifstream memInfo("/proc/meminfo");
			while (std::getline(memInfo, line)) {
				if (line.compare(0, 13, "MemAvailable:") == 0) {
					physAvailMB = std::strtod(line.c_str() + 13, nullptr) / 1024.0;
					break;
				}
			}
			time_t now = ::time(nullptr);
			struct tm tm_result;
			localtime_r(&now, &tm_result);
			char timebuf[32];
			snprintf(timebuf, sizeof(timebuf), "%02d-%02d %02d:%02d:%02d",
				tm_result.tm_mon + 1, tm_result.tm_mday,
				tm_result.tm_hour, tm_result.tm_min, tm_result.tm_sec);
			pThis->m_outputFile << timebuf << "," << workingSetMB << "," << virtualMB << ","
								<< physAvailMB << endl;
		} catch (...) {
		}
#endif

		::Sleep(pThis->m_minuteInterval * 60 * 1000);
	}

	return 0;
}
