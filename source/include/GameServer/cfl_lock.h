#ifndef _CFL_LOCK_H_
#define _CFL_LOCK_H_

#ifdef PKO_PLATFORM_WINDOWS
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#endif

class cfl_nilock {
public:
	cfl_nilock() {}
	virtual ~cfl_nilock() {}

public:
	virtual void lock() {}
	virtual bool trylock() { return true; }
	virtual void unlock() {}
};

class cfl_spinlock : public cfl_nilock {
public:
	cfl_spinlock();
	~cfl_spinlock();

public:
	void lock();
	bool trylock();
	void unlock();

private:
	CRITICAL_SECTION _cs;
};

#endif