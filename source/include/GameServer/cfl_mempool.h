
#ifndef _CFL_MEMPOOL_H_
#define _CFL_MEMPOOL_H_

#include <vector>
#include <list>
#include "cfl_lock.h"

// Forward declare logging function if available
#ifndef LG
extern void LG(const char* category, const char* format, ...);
#endif

struct cfl_mpelem;
template <class T>
class cfl_mp {
	friend struct cfl_mpelem;

public:
	cfl_mp(int max_cnt) : _eplist(), _lock() {
		_eplist.clear();
		_free.clear();

		_cnt = max(1, max_cnt);
		_ep = new T[_cnt];

		for (int i = 0; i < _cnt; ++i) {
			_ep[i]._pool = this;
			_free.push_back(&_ep[i]);
		}

		_eplist.push_back(_ep);
	}

	~cfl_mp() {
		_free.clear();

		for (auto it = _eplist.begin(); it != _eplist.end(); ++it) {
			_ep = (*it);
			delete[] _ep;
		}

		_eplist.clear();
		_cnt = 0;
	}

	T* get() {
		T* tmp;

		_lock.lock();
		try {
			tmp = (_free.empty()) ? nullptr : _free.front();
			if (tmp != nullptr)
				_free.pop_front();
			else {
				try {
					_ep = new T[_cnt];
					for (int i = 0; i < _cnt; ++i) {
						_ep[i]._pool = this;
						_free.push_back(&_ep[i]);
					}

					_eplist.push_back(_ep);

					tmp = (_free.empty()) ? nullptr : _free.front();
					if (tmp != nullptr)
						_free.pop_front();
				} catch (const std::bad_alloc& e) {
					LG("mempool_error", "Memory allocation failed in pool expansion: %s (requested %d elements)\n", e.what(), _cnt);
					tmp = nullptr;
				} catch (const std::exception& e) {
					LG("mempool_error", "Exception during pool expansion: %s\n", e.what());
					tmp = nullptr;
				} catch (...) {
					LG("mempool_error", "Unknown exception during memory pool expansion\n");
					tmp = nullptr;
				}
			}
		} catch (const std::exception& e) {
			LG("mempool_error", "Exception in memory pool get(): %s\n", e.what());
			tmp = nullptr;
		} catch (...) {
			LG("mempool_error", "Unknown exception in memory pool get()\n");
			tmp = nullptr;
		}
		_lock.unlock();

		if (tmp != nullptr) {
			try {
				tmp->on_get();
			} catch (const std::exception& e) {
				LG("mempool_error", "Exception in on_get() callback: %s\n", e.what());
			} catch (...) {
				LG("mempool_error", "Unknown exception in on_get() callback\n");
			}
		}

		return tmp;
	}

protected:
	void free(T* elem) {
		if (elem == nullptr)
			return;

		try {
			elem->on_ret();
		} catch (const std::exception& e) {
			LG("mempool_error", "Exception in on_ret() callback: %s\n", e.what());
		} catch (...) {
			LG("mempool_error", "Unknown exception in on_ret() callback\n");
		}

		_lock.lock();
		try {
			_free.push_back(elem);
		} catch (const std::exception& e) {
			LG("mempool_error", "Exception during element free: %s\n", e.what());
		} catch (...) {
			LG("mempool_error", "Unknown exception during element free\n");
		}
		_lock.unlock();
	}

private:
	T* _ep;
	int _cnt;
	std::vector<T*> _eplist;

	std::list<T*> _free;
	cfl_spinlock _lock;
};

struct cfl_mpelem {
	template <class T>
	friend class cfl_mp;

protected:
	cfl_mpelem() : _pool(nullptr) {}
	virtual ~cfl_mpelem() {}

	virtual void on_get() {}
	virtual void on_ret() {}

public:
	void free() { ((cfl_mp<cfl_mpelem>*)_pool)->free(this); }

protected:
	void* _pool;
};

#endif // _CFL_MEMPOOL_H_
