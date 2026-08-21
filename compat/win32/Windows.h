// ---------------------------------------------------------------------------
// Windows.h - minimal portable stand-in for the Win32 header
// ---------------------------------------------------------------------------
//
// Several Nexus headers declare members and signatures in terms of Win32
// typedefs (HWND for a native window, HRESULT for a driver return code, and so
// on) even in code paths that never touch the Win32 API. That is enough to make
// the entire engine unparseable off Windows.
//
// This header supplies just those typedefs, with the same sizes and semantics,
// so those declarations compile everywhere. It deliberately does NOT declare
// any Win32 *functions*: anything that genuinely needs the Windows API belongs
// behind a `#ifdef _WIN32` guard or in a platform backend, and leaving the
// functions undeclared keeps that boundary honest - a stray CreateWindowEx call
// fails to compile here rather than failing mysteriously at link time.
//
// Only reached on non-Windows builds; Windows uses the real SDK header.
//
// Note the capitalisation: this file is `Windows.h`, and there is deliberately
// no lowercase `windows.h` beside it. macOS and Windows use case-insensitive
// filesystems by default, so two files differing only in case cannot coexist in
// a checkout - one silently clobbers the other and the build fails in a way
// that never reproduces on Linux. Every non-Windows include site spells it
// `<Windows.h>`; the lowercase spellings in the codebase all sit inside
// `#ifdef _WIN32`, where the real SDK header is used instead.
//
#pragma once

#if defined(_WIN32)
#error "compat/win32/Windows.h must not be used on Windows - include the SDK header instead."
#endif

#include <cstdint>
#include <cstddef>

// --- Integral aliases -------------------------------------------------------
// Widths follow the Win32 LLP64 definitions so that struct layouts and format
// assumptions carry over unchanged.
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned int        UINT;
typedef int                 INT;
typedef unsigned int        DWORD;
typedef long                LONG;
typedef unsigned long       ULONG;
typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;
typedef int                 BOOL;
typedef float               FLOAT;
typedef std::uint64_t       UINT64;
typedef std::int64_t        INT64;
typedef std::uint32_t       UINT32;
typedef std::int32_t        INT32;
typedef std::uint16_t       UINT16;
typedef std::uint8_t        UINT8;
typedef std::size_t         SIZE_T;

typedef char                CHAR;
typedef wchar_t             WCHAR;
typedef const char*         LPCSTR;
typedef char*               LPSTR;
typedef const wchar_t*      LPCWSTR;
typedef wchar_t*            LPWSTR;
typedef void*               LPVOID;
typedef const void*         LPCVOID;

typedef std::intptr_t       INT_PTR;
typedef std::uintptr_t      UINT_PTR;
typedef std::uintptr_t      WPARAM;
typedef std::intptr_t       LPARAM;
typedef std::intptr_t       LRESULT;

// --- Opaque handles ---------------------------------------------------------
// Distinct struct pointers rather than plain void*, so the compiler still
// catches a window handle passed where a module handle was expected.
#define NEXUS_DECLARE_HANDLE(name) struct name##__; typedef struct name##__* name

NEXUS_DECLARE_HANDLE(HWND);
NEXUS_DECLARE_HANDLE(HINSTANCE);
NEXUS_DECLARE_HANDLE(HMODULE);
NEXUS_DECLARE_HANDLE(HDC);
NEXUS_DECLARE_HANDLE(HGLRC);
NEXUS_DECLARE_HANDLE(HICON);
NEXUS_DECLARE_HANDLE(HCURSOR);
NEXUS_DECLARE_HANDLE(HMENU);
NEXUS_DECLARE_HANDLE(HBRUSH);
NEXUS_DECLARE_HANDLE(HBITMAP);
NEXUS_DECLARE_HANDLE(HMONITOR);

typedef void* HANDLE;

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(std::intptr_t)-1)
#endif

// --- HRESULT ----------------------------------------------------------------
typedef long HRESULT;

#ifndef S_OK
#define S_OK            ((HRESULT)0L)
#endif
#ifndef S_FALSE
#define S_FALSE         ((HRESULT)1L)
#endif
#ifndef E_FAIL
#define E_FAIL          ((HRESULT)0x80004005L)
#endif
#ifndef E_INVALIDARG
#define E_INVALIDARG    ((HRESULT)0x80070057L)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY   ((HRESULT)0x8007000EL)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL       ((HRESULT)0x80004001L)
#endif
#ifndef E_POINTER
#define E_POINTER       ((HRESULT)0x80004003L)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr)    (((HRESULT)(hr)) < 0)
#endif

// --- Small structs ----------------------------------------------------------
typedef union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct tagPOINT { LONG x; LONG y; } POINT, *LPPOINT;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT, *LPRECT;

typedef struct tagMSG {
    HWND    hwnd;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
    DWORD   time;
    POINT   pt;
} MSG, *LPMSG;

typedef struct _GUID {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;

typedef GUID IID;
typedef GUID CLSID;
typedef const GUID& REFIID;
typedef const GUID& REFCLSID;

// --- Common constants and no-op annotations ---------------------------------
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// SAL annotations and calling conventions are Windows-only decoration.
#define WINAPI
#define APIENTRY
#define CALLBACK
#define __stdcall
#define __cdecl
#define STDMETHODCALLTYPE

#ifndef _In_
#define _In_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _In_reads_(x)
#define _Out_writes_(x)
#endif

// --- Virtual key codes ------------------------------------------------------
// Shared engine code compares key values against these names. They are plain
// numeric constants with the same values as the Windows SDK, so keeping them
// here lets that code stay platform-neutral without changing behaviour.
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_LSHIFT    0xA0
#define VK_RSHIFT    0xA1
#define VK_LCONTROL  0xA2
#define VK_RCONTROL  0xA3
