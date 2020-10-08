/*
Code copyright 2020 nori-dev-akg
Code released under the MIT license
https://opensource.org/licenses/mit-license.php
*/
// ABShutter3Key.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include <deque>
#include <shellapi.h>	// https://github.com/marek/trayframework
#include "ABShutter3Key.h"
#include "ABShutter3KeyDLL.h"
#include "SendKeys.h"

#define MAX_LOADSTRING 100

// OutputDebugString
#ifdef _DEBUG
#   define _OutputDebugString( str, ... ) \
      { \
        WCHAR c[512]; \
        swprintf_s( c, 512, str, __VA_ARGS__ ); \
		OutputDebugString( c ); \
      }
#else
#    define _OutputDebugString( str, ... )
#endif

// Various consts & Defs
#define	WM_USER_SHELLICON WM_USER + 1
#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))
NOTIFYICONDATA structNID;

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
WCHAR iOS[256], Android[256];

//----------------
// HWND of main executable
HWND mainHwnd;
// Windows message for communication between main executable and DLL module
UINT const WM_HOOK = WM_APP + 1;
UINT const WM_HOOK_LL = WM_APP + 2;
// How long should Hook processing wait for the matching Raw Input message (ms)
DWORD maxWaitingTime = 100;
// Device name of AB Shutter3
WCHAR ABShutterVIDPID[256];
WCHAR* const globalKeyboard = L"GlobalKeyboard";
BOOL RawInput_0D = false;
BOOL Andriod_LL_0D = false;
//----------------

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL                FindABShutter3();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: ここにコードを挿入してください。

	// Load settings.ini
	WCHAR path[MAX_PATH];   //パス取得用
	GetModuleFileName(NULL, path, MAX_PATH);
	WCHAR* ptmp = wcsrchr(path, '\\'); // \の最後の出現位置を取得
	*(++ptmp) = '\0';
	wcscat_s(path, L"settings.ini");
	GetPrivateProfileString(L"ABShutter3Key", L"VID", L"_VID&02248a_PID&8266", ABShutterVIDPID, 256, path);
	GetPrivateProfileString(L"ABShutter3Key", L"iOS", L"", iOS, 256, path);
	GetPrivateProfileString(L"ABShutter3Key", L"Android", L"", Android, 256, path);

	// グローバル文字列を初期化する
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_ABSHUTTER3KEY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// アプリケーション初期化の実行:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ABSHUTTER3KEY));

	// tray icon settings
	structNID.cbSize = sizeof(NOTIFYICONDATA);
	structNID.hWnd = (HWND)mainHwnd;
	structNID.uID = IDI_TRAYICON;
	structNID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	wcscpy_s(structNID.szTip, L"ABShutter3Key");
	structNID.hIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_TRAYICON));
	structNID.uCallbackMessage = WM_USER_SHELLICON;

	// Display tray icon
	if (!Shell_NotifyIcon(NIM_ADD, &structNID)) {
		MessageBox(NULL, L"Systray Icon Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	MSG msg;

	// メイン メッセージ ループ:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ABSHUTTER3KEY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ABSHUTTER3KEY);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	// Save the HWND
	mainHwnd = hWnd;

	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	// Register for receiving Raw Input for keyboards
	RAWINPUTDEVICE rawInputDevice[1];
	rawInputDevice[0].usUsagePage = 1;
	rawInputDevice[0].usUsage = 6;
	rawInputDevice[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
	rawInputDevice[0].hwndTarget = hWnd;
	if (!RegisterRawInputDevices(rawInputDevice, 1, sizeof(rawInputDevice[0]))) {
		MessageBox(NULL, L"RegisterRawInputDevices Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT lpClickPoint;

	if (message == WM_TASKBAR_CREATE) {			// Taskbar has been recreated (Explorer crashed?)
		// Display tray icon
		if (!Shell_NotifyIcon(NIM_ADD, &structNID)) {
			MessageBox(NULL, L"Systray Icon Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			DestroyWindow(hWnd);
			return -1;
		}
	}

	switch (message)
	{
		// sys tray icon Messages
	case WM_USER_SHELLICON:
		if (LOWORD(lParam) == WM_LBUTTONDOWN)
		{
			HMENU hMenu, hSubMenu;
			// get mouse cursor position x and y as lParam has the Message itself
			GetCursorPos(&lpClickPoint);

			// Load menu resource
			hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_POPUP_MENU));
			if (!hMenu)
				return -1;	// !0, message not successful?

			// Select the first submenu
			hSubMenu = GetSubMenu(hMenu, 0);
			if (!hSubMenu) {
				DestroyMenu(hMenu);        // Be sure to Destroy Menu Before Returning
				return -1;
			}

			// Display menu
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			SendMessage(hWnd, WM_NULL, 0, 0);

			// Kill off objects we're done with
			DestroyMenu(hMenu);
		}
		break;

		// Raw Input Message
	case WM_INPUT:
	{
		UINT bufferSize;

		// Prepare buffer for the data
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
		LPBYTE dataBuffer = new BYTE[bufferSize];
		// Load data into the buffer
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, dataBuffer, &bufferSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)dataBuffer;

		// Get the virtual key code of the key and report it
		USHORT virtualKeyCode = raw->data.keyboard.VKey;
		USHORT keyPressed = raw->data.keyboard.Flags & RI_KEY_BREAK ? 0 : 1;
		_OutputDebugString(L"RawInput: %X (%d)\n", virtualKeyCode, keyPressed);

		// Prepare string buffer for the device name
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
		WCHAR* stringBuffer;
		if (raw->header.hDevice && bufferSize > 0) {
			stringBuffer = new WCHAR[bufferSize];
			// Load the device name into the buffer
			GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, stringBuffer, &bufferSize);
		}
		else {
			bufferSize = (UINT)wcslen(globalKeyboard) + 1;
			stringBuffer = new WCHAR[bufferSize];
			wcscpy_s(stringBuffer, bufferSize, globalKeyboard);
		}

		if (virtualKeyCode == 0x0D && wcsstr(stringBuffer, ABShutterVIDPID) != NULL)
		{
			RawInput_0D = true;
			_OutputDebugString(L"RawInput: DecisionRecord true: %X (%s)\n", virtualKeyCode, stringBuffer);
		}
		else
		{
			_OutputDebugString(L"RawInput: DecisionRecord false: %X (%s)\n", virtualKeyCode, stringBuffer);
		}

		delete[] stringBuffer;
		delete[] dataBuffer;
		return 0;
	}
	case WM_INPUT_DEVICE_CHANGE:
	{
		if (wParam == GIDC_ARRIVAL) {
			_OutputDebugString(L"WM_INPUT_DEVICE_CHANGE GIDC_ARRIVAL\n");
		}
		else if (wParam == GIDC_REMOVAL) {
			_OutputDebugString(L"WM_INPUT_DEVICE_CHANGE GIDC_REMOVAL\n");
		}
		if (FindABShutter3()) {
			if (!InstallHook(hWnd)) {
				MessageBox(NULL, L"Install hook Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			}
		}
		else {
			UninstallHook();
		}
		return 0;
	}

	// Message from Hooking DLL
	case WM_HOOK:
	{
		USHORT virtualKeyCode = (USHORT)wParam;
		USHORT keyPressed = lParam & 0x80000000 ? 0 : 1;
		_OutputDebugString(L"Hook: %X (%d)\n", virtualKeyCode, keyPressed);

		if (virtualKeyCode == 0x0D && RawInput_0D)
		{
			CSendKeys sk;
			if (!keyPressed) { // when keyup
				sk.SendKeys(Android);
				_OutputDebugString(L"%s\n", Android);
			}
			RawInput_0D = false;
			_OutputDebugString(L"Hook: %X (%d) is being blocked!\n", virtualKeyCode, keyPressed);
			return 1;
		}
		return 0;
	}

	case WM_HOOK_LL:
	{
		DWORD virtualKeyCode = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
		DWORD flags = ((KBDLLHOOKSTRUCT*)lParam)->flags;
		USHORT keyPressed = flags & LLKHF_UP ? 0 : 1;
		_OutputDebugString(L"Hook_LL: %X (%X)\n", virtualKeyCode, keyPressed);

		if (virtualKeyCode == 0xAF)
		{
			_OutputDebugString(L"Hook_LL: %X (%d) is being blocked!\n", virtualKeyCode, keyPressed);
			// Send
			if (keyPressed) {
				if (!Andriod_LL_0D) {
					CSendKeys sk;
					sk.SendKeys(iOS);
					_OutputDebugString(L"%s\n", iOS);
				}
			}
			return 1;
		}
		if (virtualKeyCode == 0x0D) {
			Andriod_LL_0D = keyPressed ? true : false;
		}
		return 0;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
		case ID_POPUP_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: HDC を使用する描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &structNID);	// Remove Tray Item
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

BOOL FindABShutter3()
{
	// return result
	BOOL bFind = FALSE;

	// Get Number Of Devices
	UINT nDevices = 0;
	GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

	// Got Any?
	if (nDevices < 1)
	{
		// Error
		return FALSE;
	}

	// Allocate Memory For Device List
	PRAWINPUTDEVICELIST pRawInputDeviceList;
	pRawInputDeviceList = new RAWINPUTDEVICELIST[sizeof(RAWINPUTDEVICELIST) * nDevices];

	// Got Memory?
	if (pRawInputDeviceList == NULL)
	{
		// Error
		return FALSE;
	}

	// Fill Device List Buffer
	int nResult;
	nResult = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));

	// Got Device List?
	if (nResult < 0)
	{
		// Clean Up
		delete[] pRawInputDeviceList;

		// Error
		return FALSE;
	}

	// Loop Through Device List
	for (UINT i = 0; i < nDevices && !bFind; i++)
	{
		// Get Character Count For Device Name
		UINT nBufferSize = 0;
		nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, // Device
			RIDI_DEVICENAME,                // Get Device Name
			NULL,                           // NO Buff, Want Count!
			&nBufferSize);                 // Char Count Here!

										   // Got Device Name?
		if (nResult < 0)
		{
			// Next
			continue;
		}

		// Allocate Memory For Device Name
		WCHAR* wcDeviceName = new WCHAR[nBufferSize + 1];

		// Got Memory
		if (wcDeviceName == NULL)
		{
			// Next
			continue;
		}

		// Get Name
		nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, // Device
			RIDI_DEVICENAME,                // Get Device Name
			wcDeviceName,                   // Get Name!
			&nBufferSize);                 // Char Count

										   // Got Device Name?
		if (nResult < 0)
		{
			// Clean Up
			delete[] wcDeviceName;

			// Next
			continue;
		}

		// search ABShutter VID
		if (wcsstr(wcDeviceName, ABShutterVIDPID) != NULL) {
			bFind = TRUE;
		}
		/*
				// Set Device Info & Buffer Size
				RID_DEVICE_INFO rdiDeviceInfo;
				rdiDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
				nBufferSize = rdiDeviceInfo.cbSize;

				// Get Device Info
				nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice,
					RIDI_DEVICEINFO,
					&rdiDeviceInfo,
					&nBufferSize);

				// Got All Buffer?
				if (nResult < 0)
				{
					// Next
					continue;
				}

				// Keyboard
				if (rdiDeviceInfo.dwType == RIM_TYPEKEYBOARD)
				{
					// Current Device
					_OutputDebugString(L"Displaying device(%d)\n", i);
					_OutputDebugString(L"Device Name: %s\n", wcDeviceName);
					_OutputDebugString(L"Keyboard mode: %X\n", rdiDeviceInfo.keyboard.dwKeyboardMode);
					_OutputDebugString(L"Number of function keys: %X\n", rdiDeviceInfo.keyboard.dwNumberOfFunctionKeys);
					_OutputDebugString(L"Number of indicators: %X\n", rdiDeviceInfo.keyboard.dwNumberOfIndicators);
					_OutputDebugString(L"Number of keys total: %X\n", rdiDeviceInfo.keyboard.dwNumberOfKeysTotal);
					_OutputDebugString(L"Type of the keyboard: %X\n", rdiDeviceInfo.keyboard.dwType);
					_OutputDebugString(L"Subtype of the keyboard: %X\n", rdiDeviceInfo.keyboard.dwSubType);
				}
		*/
		// Delete Name Memory!
		delete[] wcDeviceName;
	}

	// Clean Up - Free Memory
	delete[] pRawInputDeviceList;

	// Exit
	return bFind;
}