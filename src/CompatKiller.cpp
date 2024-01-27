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
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <string>
#pragma comment(lib, "psapi.lib") 

// This function returns vector DWORD filled in with the current PIDs
static std::vector<DWORD> getProcIDs()
{
	std::vector<DWORD> out(200, 0); // Init size;
	try
	{	// Get the list of process identifiers.
		DWORD cbNeeded = -1;
		while (::EnumProcesses(out.data(), out.size() * sizeof DWORD, &cbNeeded) &&
							   cbNeeded >= out.size() * sizeof DWORD)
			out.resize(cbNeeded / sizeof DWORD + 1);
	}
	catch (...) // memory
	{
		out.clear();
	}
	return out;
}

static std::string getProcessName(DWORD PID)
{
	TCHAR szName[2 * MAX_PATH];
	szName[0] = 0;

	// Get a handle to the process.
	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == NULL)
		return szName;

	// Get the process name.
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

static bool isProcessRunning(LPCTSTR szName)
{
	auto processes = getProcIDs();
	if (processes.empty())
		return false;

	// Scan through the all and check for names
	for (auto ID : processes)
	{
		auto name = getProcessName(ID);
		if (_tcsicmp(name.c_str(), szName) == 0)
			return true;
	}
	return false;
}


static int help()
{
	_tprintf(_T("This application is intended to kill annoying CompatTelRunner process\n\n"));
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	_tprintf(_T("CompatKiller")
#ifdef _WIN64
		_T( " x64")
#endif
		_T(". (C) 2024 VorontSOFT. Version 1.0\n"));

	while (true)
	{
		if (isProcessRunning(_T("CompatTelRunner.exe")))
			printf("Found\n");
		_sleep(2000);
	}
	
	return help();
}
