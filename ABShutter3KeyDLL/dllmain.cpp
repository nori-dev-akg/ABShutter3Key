/*
Code copyright 2020 nori-dev-akg
Code released under the MIT license
https://opensource.org/licenses/mit-license.php
*/
// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"
#include "ABShutter3KeyDLL.h"

#pragma data_seg (".SHARED")

UINT const WM_HOOK = WM_APP + 1;
UINT const WM_HOOK_LL = WM_APP + 2;
// HWND of the main executable (managing application)
HWND hwndServer = NULL;
#pragma data_seg ()
#pragma comment (linker, "/section:.SHARED,RWS")

HINSTANCE instanceHandle;
HHOOK hookHandle;
HHOOK hookLLHandle;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		instanceHandle = hModule;
		hookHandle = NULL;
		hookLLHandle = NULL;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// Keyboard Hook procedure
static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	// Report the event to the main window. If the return value is 1, block the input; otherwise pass it along the Hook Chain
	if (code >= 0) {
		if (SendMessage(hwndServer, WM_HOOK, wParam, lParam)) {
			return 1;
		}
	}

	return CallNextHookEx(hookHandle, code, wParam, lParam);
}

static LRESULT CALLBACK KeyboardLLProc(int code, WPARAM wParam, LPARAM lParam)
{
	// Report the event to the main window. If the return value is 1, block the input; otherwise pass it along the Hook Chain
	if (code == HC_ACTION) {
		if (SendMessage(hwndServer, WM_HOOK_LL, wParam, lParam)) {
			return 1;
		}
	}

	return CallNextHookEx(hookLLHandle, code, wParam, lParam);
}

BOOL InstallHook(HWND hwndParent)
{
	if (hwndServer != NULL)
	{
		// Already hooked
		return TRUE;
	}

	// Register keyboard Hook
	hookHandle = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, instanceHandle, 0);
	hookLLHandle = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardLLProc, instanceHandle, 0);
	if (hookHandle == NULL || hookLLHandle == NULL)
	{
		return FALSE;
	}
	hwndServer = hwndParent;
	return TRUE;
}

BOOL UninstallHook()
{
	if (hookHandle != NULL)
	{
		UnhookWindowsHookEx(hookHandle);
	}
	if (hookLLHandle != NULL)
	{
		UnhookWindowsHookEx(hookLLHandle);
	}
	hwndServer = NULL;
	hookHandle = NULL;
	hookLLHandle = NULL;
	return TRUE;
}
