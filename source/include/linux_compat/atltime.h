// atltime.h - Linux shim for MFC/ATL CTime and CTimeSpan
// Provides minimal CTime/CTimeSpan compatible with AccountServer usage
#ifndef _ATLTIME_H_LINUX_COMPAT
#define _ATLTIME_H_LINUX_COMPAT

#ifdef _WIN32
#error "This shim is for Linux only"
#endif

#include <ctime>

class CTimeSpan {
public:
    CTimeSpan() : m_span(0) {}
    // CTimeSpan(seconds)
    explicit CTimeSpan(time_t span) : m_span(span) {}
    // CTimeSpan(days, hours, mins, secs)
    CTimeSpan(long days, int hours, int mins, int secs)
        : m_span(days * 86400 + hours * 3600 + mins * 60 + secs) {}

    time_t GetTotalSeconds() const { return m_span; }
    bool operator>(const CTimeSpan& other) const { return m_span > other.m_span; }
    bool operator<(const CTimeSpan& other) const { return m_span < other.m_span; }
    bool operator>=(const CTimeSpan& other) const { return m_span >= other.m_span; }
    bool operator<=(const CTimeSpan& other) const { return m_span <= other.m_span; }
    bool operator==(const CTimeSpan& other) const { return m_span == other.m_span; }

private:
    time_t m_span;
};

class CTime {
public:
    CTime() : m_time(0) {}
    CTime(time_t t) : m_time(t) {}

    static CTime GetCurrentTime() {
        return CTime(::time(nullptr));
    }

    time_t GetTime() const { return m_time; }

    CTimeSpan operator-(const CTime& other) const {
        return CTimeSpan(m_time - other.m_time);
    }

private:
    time_t m_time;
};

#endif // _ATLTIME_H_LINUX_COMPAT
