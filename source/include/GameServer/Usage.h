
#ifndef USAGE_H
#define USAGE_H

class Usage {
public:
	Usage() : m_cpusage(0) {}
	void Add(const int usage) {
		try {
			m_cpusage += usage;
		} catch (...) {
		}
	}
	int Get() {
		int l_retval;
		try {
			l_retval = m_cpusage;
			m_cpusage = 0;
		} catch (...) {
		}
		return l_retval;
	}

private:
	int m_cpusage;
};

#endif
