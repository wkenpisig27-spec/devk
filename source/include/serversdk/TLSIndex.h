#ifndef TLSINDEX_H
#define TLSINDEX_H

#include "excp.h"
#include "platform_compat.h"

_DBC_BEGIN

//==============================================================================
struct TLSIndex {
	TLSIndex() : index(TlsAlloc()) {
		if (index == TLS_OUT_OF_INDEXES) {
			throw excpThrd("�����̱߳��ش洢�������������");
		}
	}
	~TLSIndex() {
		TlsFree(index);
	}
	LPVOID GetPointer() { return TlsGetValue(index); }
	BOOL SetPointer(LPVOID p) { return TlsSetValue(index, p); }

private:
	DWORD index;
};

_DBC_END

#endif
