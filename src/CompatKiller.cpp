////////////////////////////////////////////////////////////////////////////////
// Author: Nikolai Vorontsov
// CompatKiller.cpp : Main function
//
// Copyright (c) 2024 VorontSOFT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
// copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <string>

#if defined(_UNICODE) || defined(UNICODE)
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

static std::vector<DWORD> getProcIDs()
{
	std::vector<DWORD> out(512, 0); // Init size
	try
	{
		DWORD cbReturned = 0;
		while (::EnumProcesses(out.data(), out.size() * sizeof DWORD, &cbReturned) &&
				cbReturned >= out.size() * sizeof DWORD)
			out.resize(out.size() * 2);
		out.resize(cbReturned / sizeof DWORD);
	}
	catch (...) // memory
	{
		out.clear();
	}
	return out;
}

static tstring getProcessName(DWORD PID)
{
	TCHAR szName[MAX_PATH];
	szName[0] = 0;

	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == NULL)
		return szName;

	DWORD dwLen = ::GetModuleBaseName(hProcess, 0, szName, std::size(szName));
	if (dwLen == 0 && GetLastError() == ERROR_PARTIAL_COPY)
	{	// Trying to open 64-bit process from 32-bit one
		dwLen = GetProcessImageFileName(hProcess, szName, std::size(szName));
		::CloseHandle(hProcess);
		if (dwLen > 0)
		{
			LPCTSTR pSlash = _tcsrchr(szName, _T('\\'));
			return pSlash ? pSlash + 1 : szName;
		}
	}
	else
		::CloseHandle(hProcess);
	return szName;
}

static DWORD isProcessRunning(LPCTSTR szName)
{
	for (auto PID : getProcIDs())
	{
		auto name = getProcessName(PID);
		if (_tcsicmp(name.c_str(), szName) == 0)
			return PID;
	}
	return 0;
}

class CPrivilege
{
public:
	explicit CPrivilege(LPCTSTR privilege) : hToken_(NULL), privilege_(privilege)
	{
		auto ret = ::OpenProcessToken(::GetCurrentProcess(),
									  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken_);
		if (ret)
			Set(privilege, true);
		else
			hToken_ = NULL;
	}

	~CPrivilege()
	{
		Set(privilege_.c_str(), false);
		if (hToken_)
			::CloseHandle(hToken_);
	}

	bool Set(LPCTSTR privilege, bool enable)
	{
		if (hToken_ == NULL || privilege == nullptr)
			return false;

		LUID luid;
		if (!::LookupPrivilegeValue(NULL, privilege, &luid))
			return false;

		// first pass. get current privilege setting
		TOKEN_PRIVILEGES tp; // Can use default struct as only one LUID is used
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = 0;
		TOKEN_PRIVILEGES tpPrevious;
		DWORD cbPrevious = sizeof TOKEN_PRIVILEGES;
		::AdjustTokenPrivileges(hToken_, false, &tp, sizeof TOKEN_PRIVILEGES,
								&tpPrevious, &cbPrevious);
		if (::GetLastError() != ERROR_SUCCESS)
			return false;

		// second pass. set privilege based on the previous setting
		tpPrevious.PrivilegeCount = 1;
		tpPrevious.Privileges[0].Luid = luid;

		if (enable)
			tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
		else
			tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
												tpPrevious.Privileges[0].Attributes);

		::AdjustTokenPrivileges(hToken_, false, &tpPrevious, cbPrevious, NULL, NULL);
		if (::GetLastError() != ERROR_SUCCESS)
			return false;
		return true;
	}
private:
	HANDLE hToken_;
	tstring privilege_;
};

static void killProcess(DWORD PID)
{
	CPrivilege privilege(SE_DEBUG_NAME);	// Adjust privileges

	HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, PID);
	if (hProcess)
	{
		if (::TerminateProcess(hProcess, -1))
			printf(_T("PID %u was terminated."), PID);
		::CloseHandle(hProcess);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	_tprintf(_T("CompatKiller. (C) 2024 VorontSOFT. Version 1.0\nWatching "));

	while (true)
	{
		auto PID = isProcessRunning(_T("cmd.exe"));
		if (PID != 0)
		{
			printf("\nFound PID %u, terminating...\n", PID);
			killProcess(PID);
			printf("\nWatching ");
		}
		printf(".");
		_sleep(2000);
	}

	return 0;
}
