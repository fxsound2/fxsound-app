// Win32Demo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Win32Main.h"

#include "audiopassthru.h"
#include "DfxDsp.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

												// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

AudioPassthru audioPassthru;
DfxDsp dfxDsp1;
HWND hwnd_static_1;
HWND hwnd_static_2;
HWND hwnd_static_3;

class Callee : public AudioPassthruCallback
{
public:
	void onSoundDeviceChange(std::vector<SoundDevice> sound_devices)
	{
		for (int index = 0; index < sound_devices.size(); index++)
		{
			SoundDevice sound_device = sound_devices[index];
			if (sound_device.isTargetedRealPlaybackDevice)
			{
				SetWindowTextW(hwnd_static_3, sound_device.deviceFriendlyName.c_str());
			}
		}
		
		return;
	}
};


Callee callee;

void audioPassthruTimerProc(HWND Arg1, UINT Arg2, UINT_PTR Arg3, DWORD Arg4)
{
	audioPassthru.processTimer();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WIN32MAIN, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32MAIN));

	MSG msg;

	audioPassthru.init();
	std::vector<SoundDevice> soundDevices = audioPassthru.getSoundDevices();
	audioPassthru.setDspProcessingModule(&dfxDsp1);
	audioPassthru.registerCallback(&callee);


	// Main message loop:
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
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32MAIN));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WIN32MAIN);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	::SetTimer(hWnd, 1, 100, (TIMERPROC)audioPassthruTimerProc);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	int initial_x = 10;
	int initial_y = 10;
	int button_width = 100;
	int button_height = 30;
	int horizontal_padding = 10;
	int vertical_padding = 10;


	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Power ON",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 0,         // x position 
		initial_y + (vertical_padding + button_height) * 0,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_POWER_ON_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Power OFF",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 1,         // x position 
		initial_y + (vertical_padding + button_height) * 0,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_POWER_OFF_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"EQ ON",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 0,         // x position 
		initial_y + (vertical_padding + button_height) * 1,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_EQ_ON_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"EQ OFF",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 1,         // x position 
		initial_y + (vertical_padding + button_height) * 1,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_EQ_OFF_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Preset 1",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 0,         // x position 
		initial_y + (vertical_padding + button_height) * 2,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_LOAD_PRESET_1_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Preset 2",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 1,         // x position 
		initial_y + (vertical_padding + button_height) * 2,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_LOAD_PRESET_2_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Preset 3",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		initial_x + (horizontal_padding + button_width) * 2,         // x position 
		initial_y + (vertical_padding + button_height) * 2,         // y position 
		button_width,        // Button width
		button_height,        // Button height
		hWnd,     // Parent window
		(HMENU)IDC_LOAD_PRESET_3_BUTTON,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	hwnd_static_1 = CreateWindow(
		L"STATIC",
		NULL,
		WS_VISIBLE | WS_CHILD,
		initial_x + (horizontal_padding + button_width) * 0,
		initial_y + (vertical_padding + button_height) * 3,         // y position ,
		600,
		20,
		hWnd,
		(HMENU)IDC_STATIC,
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);

	hwnd_static_2 = CreateWindow(
		L"STATIC",
		NULL,
		WS_VISIBLE | WS_CHILD,
		initial_x + (horizontal_padding + button_width) * 0,
		initial_y + (vertical_padding + button_height) * 4,         // y position ,
		600,
		20,
		hWnd,
		(HMENU)IDC_STATIC,
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);

	hwnd_static_3 = CreateWindow(
		L"STATIC",
		NULL,
		WS_VISIBLE | WS_CHILD,
		initial_x + (horizontal_padding + button_width) * 0,
		initial_y + (vertical_padding + button_height) * 5,         // y position ,
		600,
		20,
		hWnd,
		(HMENU)IDC_STATIC,
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		DfxPreset dfx_preset;
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDC_POWER_ON_BUTTON:
			dfxDsp1.powerOn(true);
			break;
		case IDC_POWER_OFF_BUTTON:
			dfxDsp1.powerOn(false);
			break;
		case IDC_EQ_ON_BUTTON:
			dfxDsp1.eqOn(true);
			break;
		case IDC_EQ_OFF_BUTTON:
			dfxDsp1.eqOn(false);
			break;
		case IDC_LOAD_PRESET_1_BUTTON:
			dfx_preset = dfxDsp1.getPresetInfo(L"C:\\Program Files (x86)\\DFX\\Universal\\Presets\\Factsoft\\1.fac");
			dfxDsp1.loadPreset(L"C:\\Program Files (x86)\\DFX\\Universal\\Presets\\Factsoft\\1.fac");
			SetWindowTextW(hwnd_static_1, dfx_preset.name.c_str());
			SetWindowTextW(hwnd_static_2, dfx_preset.full_path.c_str());
			break;
		case IDC_LOAD_PRESET_2_BUTTON:
			dfx_preset = dfxDsp1.getPresetInfo(L"C:\\Program Files (x86)\\DFX\\Universal\\Presets\\Factsoft\\10.fac");
			dfxDsp1.loadPreset(L"C:\\Program Files (x86)\\DFX\\Universal\\Presets\\Factsoft\\10.fac");
			SetWindowTextW(hwnd_static_1, dfx_preset.name.c_str());
			SetWindowTextW(hwnd_static_2, dfx_preset.full_path.c_str());
			break;
		case IDC_LOAD_PRESET_3_BUTTON:
			dfx_preset = dfxDsp1.getPresetInfo(L"C:\\Users\\X1 Carbon\\AppData\\Local\\DFX\\23\\Presets\\User\\102.usr");
			dfxDsp1.loadPreset(L"C:\\Users\\X1 Carbon\\AppData\\Local\\DFX\\23\\Presets\\User\\102.usr");
			SetWindowTextW(hwnd_static_1, dfx_preset.name.c_str());
			SetWindowTextW(hwnd_static_2, dfx_preset.full_path.c_str());
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
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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
