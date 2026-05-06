// io.h - Linux compatibility shim for MSVC io.h
// This file is only used on Linux builds.
#ifndef _IO_H_LINUX_COMPAT
#define _IO_H_LINUX_COMPAT

#if defined(_WIN32)
#error "This shim should not be used on Windows - use the real io.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// _access -> access
#ifndef _access
#define _access access
#endif

// _open / _close / _read / _write -> POSIX equivalents
#ifndef _open
#define _open open
#endif
#ifndef _close
#define _close close
#endif
#ifndef _read
#define _read read
#endif
#ifndef _write
#define _write write
#endif

// _lseek -> lseek
#ifndef _lseek
#define _lseek lseek
#endif

// _filelength - get file size by fd
#include <sys/types.h>
inline long _filelength(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) return (long)st.st_size;
    return -1L;
}

#endif // _IO_H_LINUX_COMPAT
