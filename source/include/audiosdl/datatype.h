#pragma once

#include <cstdint>

using ulong = unsigned long;

template <typename T>
class _singleton {
protected:
	_singleton() = default;
	~_singleton() = default;
	_singleton(const _singleton&) = delete;
	_singleton& operator=(const _singleton&) = delete;

public:
	static T* get_instance() {
		static T instance;
		return &instance;
	}
};

#define SINGLETON_OBJECT(Type) friend class _singleton<Type>

enum AudioResourceType {
	TYPE_WAV = 1,
	TYPE_MP3 = 2,
	TYPE_OGG = 3,
};

class SDL_Audio_Base {
public:
	SDL_Audio_Base() = default;
	virtual ~SDL_Audio_Base() = default;

	virtual bool init(const char* filename) = 0;
	virtual bool release() = 0;

	virtual bool play(bool loop = false) = 0;
	virtual bool fadeIn(int ms, bool loop = false) = 0;
	virtual bool fadeOut(int ms) = 0;
	virtual bool stop() = 0;
	virtual bool pause() = 0;
	virtual bool resume() = 0;
	virtual bool rewind() = 0;

	virtual bool is_playing() = 0;
	virtual bool is_paused() = 0;

	virtual void set_volume(int vol) { m_volume = vol; }
	virtual int get_channel() { return -1; }

	bool is_stopped() { return !is_playing() && !is_paused(); }

protected:
	int m_loop = 0;
	int m_volume = 0;
};
