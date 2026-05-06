// Stacktrace.cpp — Windows-only (SEH translator, DbgHelp stack walking)
// On Linux, crash handling is in ErrorHandler.cpp via signal + backtrace.
#ifdef _WIN32

/**
/*   Copyright (c) 2003by  Marco Welti
/*
/*   This document is  bound by the  QT Public License
/*   (http://www.trolltech.com/licenses/qpl.html).
/*   See License.txt for more information.
/*
/*
/*
/*   ALL RIGHTS RESERVED.
/*
/*   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
/*   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
/*   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/*   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
/*   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
/*   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
/*   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/*   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
/*   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
/*   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
/*   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/*
/***********************************************************************/

//#include "stdafx.h"
#include <tchar.h>

#include "Stacktrace.h"

#include <sstream>

using namespace std;

void printStacktrace(EXCEPTION_POINTERS *ep, std::ostream &os);

///////////////////////// SEHTranslator /////////////////////////////////
SEHTranslator::SEHTranslator()
: mOldHandler(_set_se_translator(translate))
{
}

SEHTranslator::~SEHTranslator()
{
	if(mOldHandler) {
		_set_se_translator(mOldHandler);
	}
}

void SEHTranslator::translate(UINT id ,EXCEPTION_POINTERS *ep)
{
	switch(id) {
		//case EXCEPTION_NO_MEMORY:					throw no_memory(pexp);				break;
	case EXCEPTION_ACCESS_VIOLATION:	ep->ExceptionRecord->ExceptionInformation [0] ? throw ReadAccessViolationException(ep) : throw WriteAccessViolationException(ep);	break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:		throw DatatypeMisalignementException(ep);	break;
	case EXCEPTION_BREAKPOINT:					    throw BreakpointException(ep);				break;
	case EXCEPTION_SINGLE_STEP:					    throw SingleStepException(ep);			break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:		throw ArrayBoundsException(ep);	break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:		throw FloatingPointException::DenormalOperandException(ep);	break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:			throw FloatingPointException::DivideByZeroException(ep);		break;
	case EXCEPTION_FLT_INEXACT_RESULT:			throw FloatingPointException::InexactResultException(ep);		break;
	case EXCEPTION_FLT_INVALID_OPERATION:		throw FloatingPointException::InvalidOperationException(ep);	break;
	case EXCEPTION_FLT_OVERFLOW:				    throw FloatingPointException::OverflowException(ep);			break;
	case EXCEPTION_FLT_STACK_CHECK:				  throw FloatingPointException::StackCheckException(ep);		break;
	case EXCEPTION_FLT_UNDERFLOW:				    throw FloatingPointException::UnderflowException(ep);			break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:			throw IntegerException::DivideByZeroException(ep);		break;
	case EXCEPTION_INT_OVERFLOW:				    throw IntegerException::OverflowException(ep);			break;
	case EXCEPTION_PRIV_INSTRUCTION:			  throw PrivilegedInstructionException(ep);		break;
	case EXCEPTION_IN_PAGE_ERROR:				    throw InPageException(ep);			break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:			throw IllegalInstructionException(ep);	break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:throw NoncontinuableException(ep); break;
	case EXCEPTION_STACK_OVERFLOW:				  throw StackOverflowException(ep);			break;
	case EXCEPTION_INVALID_DISPOSITION:			throw InvalidDispositionException(ep);	break;
	case EXCEPTION_GUARD_PAGE:					    throw GuardPageException(ep);				break;
	case EXCEPTION_INVALID_HANDLE:				  throw InvalidHandleException(ep);			break;
	}

	throw SystemException(id, ep);
}

std::string SEHTranslator::name(DWORD code)
{
	switch (code)
	{
		//case EXCEPTION_NO_MEMORY:					return "No Memory";
	case EXCEPTION_ACCESS_VIOLATION :			return "Access Violation";
	case EXCEPTION_DATATYPE_MISALIGNMENT :		return "Datatype Misalignment";
	case EXCEPTION_BREAKPOINT :					return "Breakpoint";
	case EXCEPTION_SINGLE_STEP :				return "Single Step";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED :		return "Array Bounds Exceeded";
	case EXCEPTION_FLT_DENORMAL_OPERAND :		return "Float Denormal Operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO :			return "Float Divide by Zero";
	case EXCEPTION_FLT_INEXACT_RESULT :			return "Float Inexact Result";
	case EXCEPTION_FLT_INVALID_OPERATION :		return "Float Invalid Operation";
	case EXCEPTION_FLT_OVERFLOW :				return "Float Overflow";
	case EXCEPTION_FLT_STACK_CHECK :			return "Float Stack Check";
	case EXCEPTION_FLT_UNDERFLOW :				return "Float Underflow";
	case EXCEPTION_INT_DIVIDE_BY_ZERO :			return "Integer Divide by Zero";
	case EXCEPTION_INT_OVERFLOW :				return "Integer Overflow";
	case EXCEPTION_PRIV_INSTRUCTION :			return "Privileged Instruction";
	case EXCEPTION_IN_PAGE_ERROR :				return "In Page Error";
	case EXCEPTION_ILLEGAL_INSTRUCTION :		return "Illegal Instruction";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION :	return "Noncontinuable Exception";
	case EXCEPTION_STACK_OVERFLOW :				return "Stack Overflow";
	case EXCEPTION_INVALID_DISPOSITION :		return "Invalid Disposition";
	case EXCEPTION_GUARD_PAGE :					return "Guard Page";
	case EXCEPTION_INVALID_HANDLE :				return "Invalid Handle";
	case 0xE06D7363 :							return "Microsoft C++ Exception";
	default :
		{
			stringstream text;
			text << "Unknown SEH-Exception" << code;
			return text.str();

		}
	};
}

std::string SEHTranslator::description(DWORD code)
{
	switch (code)
	{
		//case EXCEPTION_NO_MEMORY:					return "The allocation attempt failed because of a lack of available memory or heap corruption.";
	case EXCEPTION_ACCESS_VIOLATION :			return "The thread attempted to read from or write to a virtual address for which it does not have the appropriate access";
	case EXCEPTION_DATATYPE_MISALIGNMENT :		return "The thread attempted to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries, 32-bit values on 4-byte boundaries, and so on";
	case EXCEPTION_BREAKPOINT :					return "A breakpoint was encountered";
	case EXCEPTION_SINGLE_STEP :				return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED :		return "The thread attempted to access an array element that is out of bounds, and the underlying hardware supports bounds checking";
	case EXCEPTION_FLT_DENORMAL_OPERAND :		return "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO :			return "The thread attempted to divide a floating-point value by a floating-point divisor of zero";
	case EXCEPTION_FLT_INEXACT_RESULT :			return "The result of a floating-point operation cannot be represented exactly as a decimal fraction";
	case EXCEPTION_FLT_INVALID_OPERATION :		return "This exception represents any floating-point exception not included in this list";
	case EXCEPTION_FLT_OVERFLOW :				return "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type";
	case EXCEPTION_FLT_STACK_CHECK :			return "The stack overflowed or underflowed as the result of a floating-point operation";
	case EXCEPTION_FLT_UNDERFLOW :				return "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type";
	case EXCEPTION_INT_DIVIDE_BY_ZERO :			return "The thread attempted to divide an integer value by an integer divisor of zero";
	case EXCEPTION_INT_OVERFLOW :				return "The result of an integer operation caused a carry out of the most significant bit of the result";
	case EXCEPTION_PRIV_INSTRUCTION :			return "The thread attempted to execute an instruction whose operation is not allowed in the current machine mode";
	case EXCEPTION_IN_PAGE_ERROR :				return "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network";
	case EXCEPTION_ILLEGAL_INSTRUCTION :		return "The thread tried to execute an invalid instruction";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION :	return "The thread attempted to continue execution after a noncontinuable exception occurred";
	case EXCEPTION_STACK_OVERFLOW :				return "The thread used up its stack";
	case EXCEPTION_INVALID_DISPOSITION :		return "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception";
	case EXCEPTION_GUARD_PAGE :					return "Guard Page Exception";
	case EXCEPTION_INVALID_HANDLE :				return "Invalid Handle Exception";
	case 0xE06D7363 :							return "Microsoft C++ Exception";
	default :
		{
			stringstream text;
			text << "Unknown SEH-Exception" << code;
			return text.str();

		}
	};
}

///////////////////////////// Runtime stack ///////////////////////////////
RuntimeStack::RuntimeStack()
{
	gatherStack();
}

RuntimeStack::RuntimeStack(EXCEPTION_POINTERS *ep)
{
	storeStack(ep);
}

RuntimeStack::~RuntimeStack()
{
}

ostream& operator<< (ostream& os, const RuntimeStack &stack)
{
	return stack.print(os);
}

std::ostream & RuntimeStack::print(std::ostream &os) const
{
	return os << mStack << std::endl;
}

void RuntimeStack::gatherStack()
{
	__try {
		RaiseException(0,0,0,0);
	} __except(storeStack(GetExceptionInformation())) { }
}

int RuntimeStack::storeStack(EXCEPTION_POINTERS *ep)
{
	std::stringstream stacktrace;
	printStacktrace(ep, stacktrace);

	mStack = stacktrace.str();
	return EXCEPTION_CONTINUE_EXECUTION;//EXCEPTION_EXECUTE_HANDLER;
}

/////////////////////////////// TraceableException /////////////////////
TraceableException::TraceableException(const std::string &what)
: std::runtime_error(what)
{
}

TraceableException::TraceableException(const std::string &what, EXCEPTION_POINTERS *ep)
: std::runtime_error(what), mStack(ep)
{
}

const char* TraceableException::what() const
{
	std::stringstream text;
	text << runtime_error::what() << std::endl << mStack << std::endl;
	mText = text.str();
	return mText.c_str();
}

/////////////////////////////// SystemException /////////////////////
SystemException::SystemException(DWORD id, EXCEPTION_POINTERS *ep)
: TraceableException(SEHTranslator::name(id), ep) , mId(id)
{
}

const char* SystemException::what() const
{
	std::stringstream text;
	text << SEHTranslator::name(mId) << endl << SEHTranslator::description(mId) << endl << getStackTrace();
	mText = text.str();
	return getText().c_str();
}


///////////////////////////////////////////
/////////////////////////////////////////
/*
Copyright (c) 2001 - 2002
Author: Konstantin Boukreev
E-mail: konstantin@mail.primorye.ru
Created: 25.12.2001 19:41:00
Version: 1.0.2

Permission to use, copy, modify, distribute and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation.  Konstantin Boukreev makes no representations
about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

sym_engine class incapsulate Symbol Handler and Debugging Service API

the list of used API:
SymInitialize, SymCleanup, SymGetLineFromAddr, SymGetModuleBase,
SymGetSymbolInfo, SymGetSymFromAddr, SymGetSymFromName, SymGetSymNext,
SymLoadModule, SymSetOptions
StackWalk

based on code\ideas from John Robbins's book "Debugging application"
http://www.wintellect.com/robbins
*/

#ifndef _sym_engine_e4b31bc5_8e01_4cda_b5a4_905dde52ac01
#define _sym_engine_e4b31bc5_8e01_4cda_b5a4_905dde52ac01

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Win64 support added for x64 migration
// Note: Stack tracing functionality may be limited on x64
#ifdef _WIN64
#pragma message("Stacktrace: Win64 mode - using limited stack walking functionality")
#endif

#include <imagehlp.h>
#include <ostream>

extern "C" {

	WINBASEAPI BOOL WINAPI SwitchToThread(VOID);

	typedef DWORD (WINAPI *SymGetModuleBaseFn)( IN HANDLE hProcess, IN DWORD dwAddr );
	typedef BOOL  (WINAPI *SymGetSymFromAddrFn)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_SYMBOL Symbol );
	typedef BOOL  (WINAPI *StackWalkFn)( DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddress );
	typedef PVOID (WINAPI *SymFunctionTableAccessFn)( HANDLE hProcess, DWORD AddrBase );
	typedef BOOL  (WINAPI *SymGetLineFromAddrFn)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE Line );
	typedef BOOL  (WINAPI *SymCleanupFn)( IN HANDLE hProcess );
	typedef BOOL  (WINAPI *SymInitializeFn)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
	typedef DWORD (WINAPI *SymSetOptionsFn)( IN DWORD SymOptions );
	typedef DWORD (WINAPI *SymGetOptionsFn)( VOID );
	typedef DWORD (WINAPI *SymLoadModuleFn)( IN HANDLE hProcess, IN HANDLE hFile,IN PSTR ImageName, IN PSTR ModuleName, IN DWORD BaseOfDll, IN DWORD SizeOfDll );

	typedef BOOL  (WINAPI *SymGetModuleInfoFn)( IN HANDLE hProcess, IN DWORD dwAddr, OUT PIMAGEHLP_MODULE ModuleInfo );
	typedef DWORD (WINAPI *UnDecorateSymbolNameFn)( PCSTR DecoratedName, PSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags );

}

class debug_api
{
private:
	HMODULE mLib;
	std::string mPDBPath;

	SymGetModuleBaseFn        GetModuleBase;
	SymGetSymFromAddrFn       GetSymbolFromAddr;
	StackWalkFn               MyStackWalk;
	SymFunctionTableAccessFn  FunctionTableAccess;
	SymGetLineFromAddrFn      GetLineFromAddr;
	SymCleanupFn              Cleanup;
	SymInitializeFn           Initialize;
	SymSetOptionsFn           SetOptions;
	SymGetOptionsFn           GetOptions;
	SymLoadModuleFn           LoadModule;

public:
	static debug_api& instance()
	{
		static debug_api theApi("dbghelp.dll");//theApi("imagehlp.dll");
		return theApi;
	}

	HMODULE get_module_base(HANDLE process, DWORD addr)
	{ return (HMODULE)GetModuleBase(process, addr); };

	bool get_symbol(HANDLE process, DWORD addr, PDWORD displacement, PIMAGEHLP_SYMBOL symbol)
	{ return GetSymbolFromAddr( process,  addr,  displacement,  symbol) == TRUE; }

	bool get_line(HANDLE process, DWORD addr, PDWORD displacement, PIMAGEHLP_LINE line)
	{ return GetLineFromAddr(process,  addr,  displacement,  line) == TRUE; }

	bool stack_walk(HANDLE process,  HANDLE thread, LPSTACKFRAME frame, PVOID context, PREAD_PROCESS_MEMORY_ROUTINE readMemoryRoutine)
	{ 
#ifdef _WIN64
		// On x64, StackWalk64 expects DWORD64 function pointers, need to cast
		return MyStackWalk(IMAGE_FILE_MACHINE_AMD64, process, thread, frame, context, readMemoryRoutine, 
			(PFUNCTION_TABLE_ACCESS_ROUTINE64)FunctionTableAccess, (PGET_MODULE_BASE_ROUTINE64)GetModuleBase, 0) == TRUE;
#else
		return MyStackWalk(IMAGE_FILE_MACHINE_I386, process, thread, frame, context, readMemoryRoutine, FunctionTableAccess, GetModuleBase, 0) == TRUE;
#endif
	}

	DWORD set_options(DWORD options)
	{ return SetOptions(options); }

	DWORD get_options()
	{ return GetOptions(); }

	bool initialize(HANDLE process, const char* userSearchPath, bool invadeProcess)
	{ return Initialize(process, (char*)((/*userSearchPath+*/getPDBSearchPath()).c_str()), invadeProcess) == TRUE; }

	bool cleanup(HANDLE process)
	{ return Cleanup(process) == TRUE; }

	DWORD load_module(HANDLE process, HANDLE file , const char * image, const char * module, DWORD baseAddr, DWORD size)
	{ return LoadModule(process, file , (char*)image, (char*)module, baseAddr, size); }

	~debug_api()
	{ /*if(mLib != nullptr) FreeLibrary(mLib); mLib = 0;*/ }

private:
	debug_api(const std::string &libname)
	{

		mLib = LoadLibrary(libname.c_str());

		if ( mLib == nullptr)
		{
			MessageBox(0, ("failed to load " + libname).c_str(), "Error", MB_OK);
			exit(0);
		}

		Cleanup = (SymCleanupFn) GetProcAddress( mLib, "SymCleanup" );
		FunctionTableAccess = (SymFunctionTableAccessFn) GetProcAddress( mLib, "SymFunctionTableAccess" );
		GetLineFromAddr = (SymGetLineFromAddrFn) GetProcAddress( mLib, "SymGetLineFromAddr" );
		GetModuleBase = (SymGetModuleBaseFn) GetProcAddress( mLib, "SymGetModuleBase" );
		//pSGMI = (tSGMI) GetProcAddress( lib, "SymGetModuleInfo" );
		GetOptions = (SymGetOptionsFn) GetProcAddress( mLib, "SymGetOptions" );
		GetSymbolFromAddr = (SymGetSymFromAddrFn) GetProcAddress( mLib, "SymGetSymFromAddr" );
		Initialize = (SymInitializeFn) GetProcAddress( mLib, "SymInitialize" );
		SetOptions = (SymSetOptionsFn) GetProcAddress( mLib, "SymSetOptions" );
		MyStackWalk = (StackWalkFn) GetProcAddress( mLib, "StackWalk" );
		//pUDSN = (tUDSN) GetProcAddress( lib, "UnDecorateSymbolName" );
		LoadModule = (SymLoadModuleFn) GetProcAddress( mLib, "SymLoadModule" );

		if ( Cleanup == nullptr || FunctionTableAccess == nullptr || GetLineFromAddr == nullptr || GetModuleBase == nullptr ||
			GetOptions == nullptr || GetSymbolFromAddr == nullptr || Initialize == nullptr || SetOptions == nullptr ||
			MyStackWalk == nullptr || LoadModule == nullptr /*|| pSLM == NULL*/ )
		{
			MessageBox(0, ("not all required functions found in " + libname).c_str(), "Error", MB_OK);
			FreeLibrary( mLib );
			mLib = 0;
			exit(0);
		}
	}

	string getPDBSearchPath()
	{
		if(mPDBPath.empty()) {
			initializePDBPAth();
		}
		return mPDBPath;
	}

	void initializePDBPAth()
	{
		const size_t size = 8192;
		char buffer[size+1]; //8k should (hopefuly) be enough
		stringstream path;

		// current directory
		if (GetCurrentDirectoryA( size, buffer)) {
			path << buffer;
		}

		// dir with executable
		if (GetModuleFileNameA(0, buffer, size))
		{
			string tmp(buffer);
			size_t pos = tmp.find_last_of("\\/");
			if(pos != string::npos) {
				path << ";" << tmp.substr(0, pos);
			}
		}

		// environment variable _NT_SYMBOL_PATH
		const size_t envCnt = 3;
		const char *env[envCnt] = {"_NT_SYMBOL_PATH", "_NT_ALTERNATE_SYMBOL_PATH", "SYSTEMROOT"};
		for(int i = 0; i < envCnt; i++) {
			if (GetEnvironmentVariableA( env[i], buffer, size)) {
				path << ";" << buffer;
			}
		}

		mPDBPath = path.str();
	}
};


//HMODULE debug_api::mLib = nullptr;

class sym_engine
{
public:
	sym_engine (unsigned);
	~sym_engine();

	void address(unsigned a)		{ m_address = a; }
	unsigned address(void) const	{ return m_address; }

	// symbol handler queries
	unsigned module  (char *, unsigned);
	unsigned symbol  (char *, unsigned, unsigned * = 0);
	unsigned fileline(char *, unsigned, unsigned *, unsigned * = 0);

	// stack walk
	bool stack_first (CONTEXT* pctx);
	bool stack_next  ();

	/*
	format argument
	%m  : module
	%f  : file
	%l  : line
	%d : line's displacement
	%s  : symbol
	%sd : symbol's displacement

	for example
	"%f(%l) : %m at %s\n"
	"%m.%s + %sd bytes, in %f:line %l\n"
	*/
	static bool stack_trace(std::ostream&, CONTEXT *, unsigned skip = 0, const char * fmt = default_fmt());
	static bool stack_trace(std::ostream&, unsigned skip = 1, const char * fmt = default_fmt());
	static bool stack_trace(std::ostream&, sym_engine&, CONTEXT *, unsigned skip = 1, const char * fmt = default_fmt());

private:
	static const char * default_fmt() { return "%f(%l) : %m at %s\n"; }
	static bool get_line_from_addr (HANDLE, unsigned, unsigned *, IMAGEHLP_LINE *);
	static unsigned get_module_basename (HMODULE, char *, unsigned);

	bool check();

private:
	unsigned		m_address;
	bool			m_ok;
	STACKFRAME *	m_pframe;
	CONTEXT *		m_pctx;

private:
	class guard
	{
	private:
		guard();
	public:
		~guard();
		bool init();
		bool fail() const { return m_ref == -1; }
		static guard & instance()
		{
			static guard g;
			return g;
		}
	private:
		void clear();
		bool load_module(HANDLE, HMODULE);
		int  m_ref;
	};
};

#endif //_sym_engine_e4b31bc5_8e01_4cda_b5a4_905dde52ac01

void printStacktrace(EXCEPTION_POINTERS *ep, std::ostream &os)
{
	sym_engine::stack_trace(os, ep->ContextRecord);
}

/*
Copyright (c) 2001 - 2002
Author: Konstantin Boukreev
E-mail: konstantin@mail.primorye.ru
Created: 25.12.2001 19:41:07
Version: 1.0.2

Permission to use, copy, modify, distribute and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation.  Konstantin Boukreev makes no representations
about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

*/

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif // _WIN32_WINNT

#include <winbase.h>
#include <crtdbg.h>
#include <malloc.h>
#include <tlhelp32.h>


//#pragma comment (lib, "dbghelp")

#ifdef VERIFY
#undef VERIFY
#endif // VERIFY

#ifdef _DEBUG
#define VERIFY(x) _ASSERTE(x)
#else
#define VERIFY(x) (x)
#endif //_DEBUG

#define WORK_AROUND_SRCLINE_BUG

#ifdef _DEBUG
//#if 1
// #define SYM_ENGINE_TRACE_SPIN_COUNT
#endif //_DEBUG

///////////////////////////////////////////////////////////////////////

bool IsNT()
{
#if 1
	OSVERSIONINFO vi = { sizeof(vi)};
	::GetVersionEx(&vi);
	return vi.dwPlatformId == VER_PLATFORM_WIN32_NT;
#else
	return false;
#endif
}

HANDLE SymGetProcessHandle()
{
	if (IsNT())
		//	if (0)
		return GetCurrentProcess();
	else
		return (HANDLE)GetCurrentProcessId();
}

BOOL __stdcall My_ReadProcessMemory (HANDLE, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
#ifdef _WIN64
	SIZE_T bytesRead;
	BOOL result = ReadProcessMemory(GetCurrentProcess(), lpBaseAddress, lpBuffer, nSize, &bytesRead);
	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = (DWORD)bytesRead;
	return result;
#else
	return ReadProcessMemory(GetCurrentProcess(), lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
#endif
}

///////////////////////////////////////////////////////////////////////

sym_engine::sym_engine (unsigned address)
: m_address(address), m_ok(false), m_pframe(0)
{
}

sym_engine::~sym_engine()
{
	//	if (m_ok) guard::instance().clear();
	delete m_pframe;
}

unsigned sym_engine::module(char * buf, unsigned len)
{
	if (!len || !buf || IsBadWritePtr(buf, len))
		return 0;

	if (!check())
		return 0;

	HANDLE hProc = SymGetProcessHandle();
	HMODULE hMod = debug_api::instance().get_module_base (hProc, m_address);
	if (!hMod) return 0;
	return get_module_basename(hMod, buf, len);
}

unsigned sym_engine::symbol(char * buf, unsigned len, unsigned * pdisplacement)
{
	if (!len || !buf ||
		IsBadWritePtr(buf, len) ||
		(pdisplacement && IsBadWritePtr(pdisplacement, sizeof(unsigned))))
		return 0;

	if (!check())
		return 0;

	BYTE symbol [ 512 ] ;
	PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)&symbol;
	memset(pSym, 0, sizeof(symbol)) ;
	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) ;
	pSym->MaxNameLength = sizeof(symbol) - sizeof(IMAGEHLP_SYMBOL);

	HANDLE hProc = SymGetProcessHandle();
	DWORD displacement = 0;
	int r = debug_api::instance().get_symbol(hProc, m_address, &displacement, pSym);
	if (!r) return 0;
	if (pdisplacement)
		*pdisplacement = displacement;

	r = _snprintf(buf, len, "%s()", pSym->Name);

	r = r == -1 ? len - 1 : r;
	buf[r] = 0;
	return r;
}

unsigned sym_engine::fileline (char * buf, unsigned len, unsigned * pline, unsigned * pdisplacement)
{
	if (!len || !buf ||
		IsBadWritePtr(buf, len) ||
		(pline && IsBadWritePtr(pline, sizeof(unsigned))) ||
		(pdisplacement && IsBadWritePtr(pdisplacement, sizeof(unsigned))))
		return 0;

	if (!check())
		return 0;

	IMAGEHLP_LINE img_line;
	memset(&img_line, 0, sizeof(IMAGEHLP_LINE));
	img_line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

	HANDLE hProc = SymGetProcessHandle();
	unsigned displacement = 0;
	if (!get_line_from_addr(hProc, m_address, &displacement, &img_line))
		return 0;
	if (pdisplacement)
		*pdisplacement = displacement;
	if (pline)
		*pline = img_line.LineNumber;
	lstrcpynA(buf, img_line.FileName, len);
	return lstrlenA(buf);
}

bool sym_engine::stack_first (CONTEXT* pctx)
{
	if (!pctx || IsBadReadPtr(pctx, sizeof(CONTEXT)))
		return false;

	if (!check())
		return false;

	if (!m_pframe)
	{
		m_pframe = new STACKFRAME;
		if (!m_pframe) return false;
	}

	memset(m_pframe, 0, sizeof(STACKFRAME));

#ifdef _X86_
	m_pframe->AddrPC.Offset       = pctx->Eip;
	m_pframe->AddrPC.Mode         = AddrModeFlat;
	m_pframe->AddrStack.Offset    = pctx->Esp;
	m_pframe->AddrStack.Mode      = AddrModeFlat;
	m_pframe->AddrFrame.Offset    = pctx->Ebp;
	m_pframe->AddrFrame.Mode      = AddrModeFlat;
#else
	// x64 architecture uses different register names
#ifdef _WIN64
	m_pframe->AddrPC.Offset       = (DWORD64)pctx->Rip;
	m_pframe->AddrPC.Mode         = AddrModeFlat;
	m_pframe->AddrReturn.Offset   = (DWORD64)pctx->Rip;  // x64 doesn't have separate return address register
	m_pframe->AddrReturn.Mode     = AddrModeFlat;
	m_pframe->AddrStack.Offset    = (DWORD64)pctx->Rsp;
	m_pframe->AddrStack.Mode      = AddrModeFlat;
	m_pframe->AddrFrame.Offset    = (DWORD64)pctx->Rbp;
	m_pframe->AddrFrame.Mode      = AddrModeFlat;
#else
	// MIPS or other architecture (original code)
	m_pframe->AddrPC.Offset       = (DWORD)pctx->Fir;
	m_pframe->AddrPC.Mode         = AddrModeFlat;
	m_pframe->AddrReturn.Offset   = (DWORD)pctx->IntRa;
	m_pframe->AddrReturn.Mode     = AddrModeFlat;
	m_pframe->AddrStack.Offset    = (DWORD)pctx->IntSp;
	m_pframe->AddrStack.Mode      = AddrModeFlat;
	m_pframe->AddrFrame.Offset    = (DWORD)pctx->IntFp;
	m_pframe->AddrFrame.Mode      = AddrModeFlat;
#endif
#endif

	m_pctx = pctx;
	return stack_next();
}

bool sym_engine::stack_next  ()
{
	if (!m_pframe || !m_pctx)
	{
		_ASSERTE(0);
		return false;
	}

	if (!m_ok)
	{
		_ASSERTE(0);
		return false;
	}

	SetLastError(0);
	HANDLE hProc = SymGetProcessHandle();
	BOOL r = debug_api::instance().stack_walk (
		hProc,
		GetCurrentThread(),
		m_pframe,
		m_pctx,
		(PREAD_PROCESS_MEMORY_ROUTINE)My_ReadProcessMemory);

	if (!r ||
		!m_pframe->AddrFrame.Offset)
	{
		return false;
	}

	// "Debugging Applications" John Robbins
	// Before I get too carried away and start calculating
	// everything, I need to double-check that the address returned
	// by StackWalk really exists. I've seen cases in which
	// StackWalk returns TRUE but the address doesn't belong to
	// a module in the process.
	DWORD dwModBase = (DWORD)debug_api::instance().get_module_base(hProc, m_pframe->AddrPC.Offset);
	if (!dwModBase)
	{
		_ASSERTE(0);
		return false;
	}

	address(m_pframe->AddrPC.Offset);
	return true;
}

bool sym_engine::get_line_from_addr (HANDLE hProc, unsigned addr, unsigned * pdisplacement, IMAGEHLP_LINE * pLine)
{
#ifdef WORK_AROUND_SRCLINE_BUG

	// "Debugging Applications" John Robbins
	// The problem is that the symbol engine finds only those source
	// line addresses (after the first lookup) that fall exactly on
	// a zero displacement. I'll walk backward 100 bytes to
	// find the line and return the proper displacement.
	DWORD displacement = 0 ;
	while (!debug_api::instance().get_line(hProc, addr - displacement, (PDWORD)pdisplacement, pLine))
	{
		if (100 == ++displacement)
			return false;
	}

	// "Debugging Applications" John Robbins
	// I found the line, and the source line information is correct, so
	// change the displacement if I had to search backward to find the source line.
	if (displacement)
		*pdisplacement = displacement;
	return true;

#else
	return 0 != GetLineFromAddr (hProc, addr, (DWORD *) pdisplacement, pLine);
#endif
}

unsigned sym_engine::get_module_basename (HMODULE hMod, char * buf, unsigned len)
{
	char filename[MAX_PATH];
	DWORD r = GetModuleFileNameA(hMod, filename, MAX_PATH);
	if (!r) return 0;

	char * p = 0;

	// Find the last '\' mark.
	int i = r - 1;
	for (; i >= 0; i--)
	{
		if (filename[i] == '\\')
		{
			p = &filename[i + 1];
			break;
		}
	}

	if (!p)
	{
		i = 0;
		p = filename;
	}


	len = (len - 1 < r - i - 1) ? len - 1 : r - i - 1;
	//	len = min(len - 1, r - i - 1);
	memcpy(buf, p, len);
	buf[len] = 0;
	return len;
}

bool sym_engine::check()
{
	if (!m_ok)
		m_ok = guard::instance().init();
	return m_ok;
}

sym_engine::guard::guard()
: m_ref(0)
{}

sym_engine::guard::~guard()
{
	clear();
}

bool sym_engine::guard::init()
{
	if (!m_ref)
	{
		m_ref = -1;

		HANDLE hProc = SymGetProcessHandle();
		DWORD  dwPid = GetCurrentProcessId();

		// initializes
		debug_api::instance().set_options (debug_api::instance().get_options()|SYMOPT_DEFERRED_LOADS|SYMOPT_LOAD_LINES);
		//	SymSetOptions (SYMOPT_UNDNAME|SYMOPT_LOAD_LINES);
		if (debug_api::instance().initialize(hProc, 0, TRUE))
		{
			// enumerate modules
			if (IsNT())
			{
				typedef BOOL (WINAPI *ENUMPROCESSMODULES)(HANDLE, HMODULE*, DWORD, LPDWORD);

				HINSTANCE hInst = LoadLibrary(_T("psapi.dll"));
				if (hInst)
				{
					ENUMPROCESSMODULES fnEnumProcessModules =
						(ENUMPROCESSMODULES)GetProcAddress(hInst, "EnumProcessModules");
					DWORD cbNeeded = 0;
					if (fnEnumProcessModules &&
						fnEnumProcessModules(GetCurrentProcess(), 0, 0, &cbNeeded) &&
						cbNeeded)
					{
						HMODULE * pmod = (HMODULE *)alloca(cbNeeded);
						DWORD cb = cbNeeded;
						if (fnEnumProcessModules(GetCurrentProcess(), pmod, cb, &cbNeeded))
						{
							m_ref = 0;
							for (unsigned i = 0; i < cb / sizeof (HMODULE); ++i)
							{
								if (!load_module(hProc, pmod[i]))
								{
									//	m_ref = -1;
									//	break;
									_ASSERTE(0);
								}
							}
						}
					}
					else
					{
						_ASSERTE(0);
					}
					VERIFY(FreeLibrary(hInst));
				}
				else
				{
					_ASSERTE(0);
				}
			}
			else
			{
				typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD, DWORD);
				typedef BOOL (WINAPI *MODULEWALK)(HANDLE, LPMODULEENTRY32);

				HMODULE hMod = GetModuleHandle(_T("kernel32"));
				CREATESNAPSHOT fnCreateToolhelp32Snapshot = (CREATESNAPSHOT)GetProcAddress(hMod, "CreateToolhelp32Snapshot");
				MODULEWALK fnModule32First = (MODULEWALK)GetProcAddress(hMod, "Module32First");
				MODULEWALK fnModule32Next  = (MODULEWALK)GetProcAddress(hMod, "Module32Next");

				if (fnCreateToolhelp32Snapshot &&
					fnModule32First &&
					fnModule32Next)
				{
					HANDLE hModSnap = fnCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
					if (hModSnap)
					{
						MODULEENTRY32 me32 = {0};
						me32.dwSize = sizeof(MODULEENTRY32);
						if (fnModule32First(hModSnap, &me32))
						{
							m_ref = 0;
							do
							{
								if (!load_module(hProc, me32.hModule))
								{
									//	m_ref = -1;
									//	break;
								}
							}
							while(fnModule32Next(hModSnap, &me32));
						}
						VERIFY(CloseHandle(hModSnap));
					}
				}
			}

			if (m_ref == -1)
			{
				VERIFY(debug_api::instance().cleanup(SymGetProcessHandle()));
			}
		}
		else
		{
			_ASSERTE(0);
		}
	}
	if (m_ref == -1)
		return false;
	if (0 == m_ref)
		++m_ref; // lock it once
	//	++m_ref;
	return true;
}

void sym_engine::guard::clear()
{
	if (m_ref ==  0) return;
	if (m_ref == -1) return;
	if (--m_ref == 0)
	{
		VERIFY(debug_api::instance().cleanup(SymGetProcessHandle()));
	}
}

bool sym_engine::guard::load_module(HANDLE hProcess, HMODULE hMod)
{
	char filename[MAX_PATH];
	if (!GetModuleFileNameA(hMod, filename, MAX_PATH))
		return false;

	HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	// "Debugging Applications" John Robbins
	// For whatever reason, SymLoadModule can return zero, but it still loads the modules. Sheez.
	SetLastError(ERROR_SUCCESS);
	if (!debug_api::instance().load_module(hProcess, hFile, filename, 0, (DWORD)hMod, 0) &&
		ERROR_SUCCESS != GetLastError())
	{
		return false;
	}

	return true;
}

bool sym_engine::stack_trace(std::ostream& os, CONTEXT * pctx, unsigned skip, const char * fmt)
{
	if (!fmt) return false;
	sym_engine sym(0);
	return stack_trace(os, sym, pctx, skip, fmt);
}

/////////////////////////////////////////////
// prints a current thread's stack

struct current_context : CONTEXT
{
	HANDLE	thread;
	volatile int signal;
};

static DWORD WINAPI tproc(void * pv)
{
	current_context * p = reinterpret_cast<current_context*>(pv);

	__try
	{
		// Konstantin, 14.01.2002 17:21:32
		// must wait in spin lock until main thread will leave a ResumeThread (must return back to user context)
		unsigned debug_only = 0;
		while (p->signal)
		{
			if (!SwitchToThread())
				Sleep(20); // forces switch to another thread
			++debug_only;
		}
#ifdef SYM_ENGINE_TRACE_SPIN_COUNT
		char s[256];
		wsprintf(s, "sym_engine::tproc, spin count %u\n", debug_only);
		OutputDebugString(s);
#endif;

		if (-1 == SuspendThread(p->thread))
		{
			p->signal  = -1;
			__leave;
		}

		__try
		{
			p->signal = GetThreadContext(p->thread, p) ? 1 : -1;
		}
		__finally
		{
			VERIFY(-1 != ResumeThread(p->thread));
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		p->signal  = -1;
	}
	return 0;
}

bool sym_engine::stack_trace(std::ostream& os, unsigned skip, const char * fmt)
{
	if (!fmt) return false;

	// attempts to get current thread's context

	current_context ctx;
	memset(&ctx, 0, sizeof current_context);

	BOOL r = DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
		&ctx.thread, 0, 0, DUPLICATE_SAME_ACCESS);

	_ASSERTE(r);
	_ASSERTE(ctx.thread);

	if (!r || !ctx.thread)
		return false;

	ctx.ContextFlags = CONTEXT_CONTROL; // CONTEXT_FULL;
	ctx.signal = -1;

	DWORD dummy;
	HANDLE worker = CreateThread(0, 0, tproc, &ctx, CREATE_SUSPENDED, &dummy);
	_ASSERTE(worker);

	if (worker)
	{
		VERIFY(SetThreadPriority(worker, THREAD_PRIORITY_ABOVE_NORMAL)); //  THREAD_PRIORITY_HIGHEST
		if (-1 != ResumeThread(worker))
		{
			unsigned debug_only = 0;
			// Konstantin, 14.01.2002 17:21:32
			ctx.signal = 0;				// only now the worker thread can get this thread context
			while (!ctx.signal)
				++debug_only; // wait in spin
#ifdef SYM_ENGINE_TRACE_SPIN_COUNT
			char s[256];
			wsprintf(s, "sym_engine::stack_trace, spin count %u\n", debug_only);
			OutputDebugString(s);
#endif
		}
		else
		{
			VERIFY(TerminateThread(worker, 0));
		}

		VERIFY(CloseHandle(worker));
	}

	VERIFY(CloseHandle(ctx.thread));

	if (ctx.signal == -1)
	{
		_ASSERTE(0);
		return false;
	}

	// now it can print stack
	sym_engine sym(0);
	stack_trace(os, sym, &ctx, skip, fmt);
	return true;
}

bool sym_engine::stack_trace(std::ostream& os, sym_engine& sym,
							 CONTEXT * pctx, unsigned skip, const char * fmt)
{
	if (!sym.stack_first(pctx))
		return false;

	char buf [512] = {0};
	char fbuf[512] = {0};
	char sbuf[512] = {0};

	os << std::dec;

	do
	{
		if (!skip)
		{
			unsigned ln = 0;
			unsigned ld = 0;
			unsigned sd = 0;
			const char* pf = 0;
			const char* ps = 0;

			for (char * p = (char *)fmt; *p; ++p)
			{
				if (*p == '%')
				{
					++p; // skips '%'
					char c = *p;
					switch (c)
					{
					case 'm':
						os << (sym.module(buf, sizeof(buf)) ? buf : "?.?");
						break;
					case 'f':
						if (!pf)
							pf = (sym.fileline(fbuf, sizeof(fbuf), &ln, &ld)) ? fbuf : " ";
						os << pf;
						break;
					case 'l':
						if (!pf)
							pf = (sym.fileline(fbuf, sizeof(fbuf), &ln, &ld)) ? fbuf : " ";
						if (*(p + 1) == 'd') { os << ld; ++p; }
						else os << ln;
						break;
					case 's':
						if (!ps)
							ps = sym.symbol(sbuf, sizeof(sbuf), &sd) ? sbuf : "?()";
						if (*(p + 1) == 'd') { os << sd; ++p; }
						else os << ps;
						break;
					case '%':
						os << '%';
						break;
					default:
						os << '%' << c;	// prints unknown format's argument
						break;
					}
				}
				else
				{
					os << *p;
				}
			}
		}
		else
		{
			--skip;
		}
	}
	while (os.good() && sym.stack_next());
	return true;
}

#endif // _WIN32
