#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <tlhelp32.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <shlobj.h>
#include <tchar.h>
#include <winternl.h>
#include <iostream>
#include <winnt.h>
typedef NTSTATUS(NTAPI* RtlAdjustPrivilege_t)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
typedef NTSTATUS(NTAPI* NtShutdownSystem_t)(ULONG);
typedef NTSTATUS(NTAPI* NtSetSystemPowerState_t)(ULONG, ULONG, ULONG);

typedef enum _SHUTDOWN_ACTION {
	ShutdownNoReboot = 0,
	ShutdownReboot = 1,
	ShutdownPowerOff = 2,
	ShutdownRestartBootLoader = 3,
	ShutdownBugCheck = 4,
	ShutdownShutdown = 5
} SHUTDOWN_ACTION;

bool ForceShutdownComputer()
{
	HMODULE hNtDll = LoadLibraryW(L"ntdll.dll");
	if (!hNtDll) {
		std::cerr << "Failed to load ntdll.dll\n";
		return false;
	}

	auto RtlAdjustPrivilege = reinterpret_cast<RtlAdjustPrivilege_t>(GetProcAddress(hNtDll, "RtlAdjustPrivilege"));
	auto NtShutdownSystem = reinterpret_cast<NtShutdownSystem_t>(GetProcAddress(hNtDll, "NtShutdownSystem"));
	auto NtSetSystemPowerState = reinterpret_cast<NtSetSystemPowerState_t>(GetProcAddress(hNtDll, "NtSetSystemPowerState"));

	if (RtlAdjustPrivilege)
	{
		BOOLEAN wasEnabled = FALSE;
		NTSTATUS status = RtlAdjustPrivilege(19 /* SeShutdownPrivilege */, TRUE, FALSE, &wasEnabled); //DO NOT DELETE /* SeShutdownPrivilege */ it will cause shitty ass error
		if (status != 0)
		{
			MessageBoxW(NULL, L"I'm not allowed to adjust my shutdown privilege.\nYou're doing something here, aren't you?!", L"Can't Modify RtlAdjustPrivilege", MB_OK | MB_ICONERROR);
			FreeLibrary(hNtDll);
			return false;
		}
	}

	if (NtSetSystemPowerState)
	{
		NTSTATUS status = NtSetSystemPowerState(PowerActionShutdownOff, PowerSystemShutdown,
			SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_POWER_SUPPLY);

		if (status == 0)
		{
			FreeLibrary(hNtDll);
			return true;
		}
	}

	if (NtShutdownSystem)
	{
		NTSTATUS status = NtShutdownSystem(ShutdownPowerOff);
		if (status == 0)
		{
			FreeLibrary(hNtDll);
			return true;
		}
	}

	BOOL success = ExitWindowsEx(EWX_POWEROFF, EWX_FORCE);
	if (!success)
	{
		MessageBoxW(NULL, L"I can't Shut off this stupid pc.", L"Lacking Permission", MB_OK | MB_ICONERROR);
		FreeLibrary(hNtDll);
		return false;
	}

	FreeLibrary(hNtDll);
	return true;
}
