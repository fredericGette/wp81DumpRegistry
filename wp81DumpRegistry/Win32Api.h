#pragma once

// See https://github.com/tandasat/SecRuntimeSample/blob/master/SecRuntimeSampleNative/Win32Api.h

#define LOGON32_LOGON_INTERACTIVE       2
#define LOGON32_LOGON_NETWORK           3
#define LOGON32_LOGON_BATCH             4
#define LOGON32_LOGON_SERVICE           5
#define LOGON32_LOGON_UNLOCK            7
#define LOGON32_LOGON_NETWORK_CLEARTEXT 8
#define LOGON32_LOGON_NEW_CREDENTIALS   9

#define LOGON32_PROVIDER_DEFAULT    0
#define LOGON32_PROVIDER_WINNT35    1
#define LOGON32_PROVIDER_WINNT40    2
#define LOGON32_PROVIDER_WINNT50    3
#define LOGON32_PROVIDER_VIRTUAL    4

typedef ACCESS_MASK REGSAM;

typedef struct _STARTUPINFOA {
	DWORD cb;
	LPSTR lpReserved;
	LPSTR lpDesktop;
	LPSTR lpTitle;
	DWORD dwX;
	DWORD dwY;
	DWORD dwXSize;
	DWORD dwYSize;
	DWORD dwXCountChars;
	DWORD dwYCountChars;
	DWORD dwFillAttribute;
	DWORD dwFlags;
	WORD wShowWindow;
	WORD cbReserved2;
	LPBYTE lpReserved2;
	HANDLE hStdInput;
	HANDLE hStdOutput;
	HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;


extern "C" {
	WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);

	LONG WINAPI RegOpenKeyExW(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
	LONG WINAPI RegQueryValueExW(HKEY, LPCWSTR, PDWORD, PDWORD, LPBYTE, PDWORD);
	LONG WINAPI RegCloseKey(HKEY);
	LONG WINAPI RegQueryInfoKeyW(HKEY, LPWSTR, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PDWORD, PFILETIME);
	LONG WINAPI RegEnumKeyExW(HKEY, DWORD, LPWSTR, PDWORD, PDWORD, LPWSTR, PDWORD, PFILETIME);
	LONG WINAPI RegEnumValueW(HKEY, DWORD, LPWSTR, PDWORD, PDWORD, PDWORD, LPBYTE, PDWORD);

	WINBASEAPI HANDLE WINAPI FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
	WINBASEAPI BOOL WINAPI FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
	WINBASEAPI BOOL WINAPI FindClose(HANDLE hFindFile);
	WINBASEAPI HANDLE WINAPI CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	WINBASEAPI BOOL WINAPI WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

	WINBASEAPI BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	WINBASEAPI BOOL WINAPI CloseHandle(HANDLE hObject);

	BOOL WINAPI LogonUserExExW(LPTSTR lpszUsername, LPTSTR lpszDomain, LPTSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PTOKEN_GROUPS pTokenGroups, PHANDLE phToken, PSID *ppLogonSid, PVOID *ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits);
}

#define WIN32API_TOSTRING(x) #x

// Link exported function
#define WIN32API_INIT_PROC(Module, Name)  \
  Name(reinterpret_cast<decltype(&::Name)>( \
      ::GetProcAddress((Module), WIN32API_TOSTRING(Name))))

// Convenientmacro to declare function
#define WIN32API_DEFINE_PROC(Name) const decltype(&::Name) Name

class Win32Api {

private:
	// Returns a base address of KernelBase.dll
	static HMODULE GetKernelBase() {
		return GetBaseAddress(&::DisableThreadLibraryCalls);
	}

	// Returns a base address of the given address
	static HMODULE GetBaseAddress(const void *Address) {
		MEMORY_BASIC_INFORMATION mbi = {};
		if (!::VirtualQuery(Address, &mbi, sizeof(mbi))) {
			return nullptr;
		}
		const auto mz = *reinterpret_cast<WORD *>(mbi.AllocationBase);
		if (mz != IMAGE_DOS_SIGNATURE) {
			return nullptr;
		}
		return reinterpret_cast<HMODULE>(mbi.AllocationBase);
	}

public:
	const HMODULE m_Kernelbase;
	WIN32API_DEFINE_PROC(GetModuleHandleW);
	WIN32API_DEFINE_PROC(RegOpenKeyExW);
	WIN32API_DEFINE_PROC(RegQueryValueExW);
	WIN32API_DEFINE_PROC(RegCloseKey);
	WIN32API_DEFINE_PROC(RegQueryInfoKeyW);
	WIN32API_DEFINE_PROC(RegEnumKeyExW);
	WIN32API_DEFINE_PROC(RegEnumValueW);
	WIN32API_DEFINE_PROC(FindFirstFileW);
	WIN32API_DEFINE_PROC(FindNextFileW);
	WIN32API_DEFINE_PROC(FindClose);
	WIN32API_DEFINE_PROC(CreateFileW);
	WIN32API_DEFINE_PROC(WriteFile);
	WIN32API_DEFINE_PROC(CreateProcessA);
	WIN32API_DEFINE_PROC(CloseHandle);
	const HMODULE m_Sspicli;
	WIN32API_DEFINE_PROC(LogonUserExExW);

	Win32Api()
		: m_Kernelbase(GetKernelBase()),
		WIN32API_INIT_PROC(m_Kernelbase, GetModuleHandleW),
		WIN32API_INIT_PROC(m_Kernelbase, RegOpenKeyExW),
		WIN32API_INIT_PROC(m_Kernelbase, RegQueryValueExW),
		WIN32API_INIT_PROC(m_Kernelbase, RegCloseKey),
		WIN32API_INIT_PROC(m_Kernelbase, RegQueryInfoKeyW),
		WIN32API_INIT_PROC(m_Kernelbase, RegEnumKeyExW),
		WIN32API_INIT_PROC(m_Kernelbase, RegEnumValueW),
		WIN32API_INIT_PROC(m_Kernelbase, FindFirstFileW),
		WIN32API_INIT_PROC(m_Kernelbase, FindNextFileW),
		WIN32API_INIT_PROC(m_Kernelbase, FindClose),
		WIN32API_INIT_PROC(m_Kernelbase, CreateFileW),
		WIN32API_INIT_PROC(m_Kernelbase, WriteFile),
		WIN32API_INIT_PROC(m_Kernelbase, CreateProcessA),
		WIN32API_INIT_PROC(m_Kernelbase, CloseHandle),
		m_Sspicli(GetModuleHandleW(L"SSPICLI.DLL")),
		WIN32API_INIT_PROC(m_Sspicli, LogonUserExExW)
	{};

};
