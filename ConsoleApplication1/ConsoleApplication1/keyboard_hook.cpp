/*
 * [KEYBOARD HOOK]
 * Recommended usage is to start the keyboard hook for however long is a separate thread, then call releaseHook from the main thread when the keyboard inputs are sanitised. See example below:

 * int main() {
 *    std::thread t1(keyboardHook);
 *    t1.detach();
 *    Sleep(10000);
 *    releaseHook();
 *    replayStoredKeystrokes();

 *    return 0;
 * }
 */

#include "keyboard.h"
#include "stats.h"

HHOOK ghHook;
KBDLLHOOKSTRUCT kbdStruct;
std::vector<INPUT> vInputs;

// flag to toggle whether input is let through
bool ALLOW_INPUT = false;
bool INPUT_BELOW_THRESHOLD = true;
int INPUT_LEN = 0;


bool setHook() {

	// set the keyboard hook for all processes on the computer
	// runs the hookCallback function when hooked is triggered
	if (!(ghHook = SetWindowsHookExW(WH_KEYBOARD_LL, hookCallback, NULL, 0))) {
		wprintf(L"%s: %d\n", L"Hooked failed to install with error code", GetLastError());
		return 1;
	}
	else {
		wprintf(L"Hook successfully installed!\n");
	}

	return 0;
}

bool releaseHook() {

	return UnhookWindowsHookEx(ghHook);
}

LRESULT __stdcall hookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	// function handler for all keyboard strokes

	// If nCode is less than zero, the hook procedure must pass the
	// message to the CallNextHookEx function without further processing
	// and should return the value returned by CallNextHookEx.
	if (nCode < 0) {
		return CallNextHookEx(ghHook, nCode, wParam, lParam);
	}

	// Retrieve name of process that the keystroke is meant for

	HWND hWnd = GetForegroundWindow();
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	WCHAR lpBaseName[MAX_PATH];
	GetModuleBaseNameW(hProc, NULL, lpBaseName, MAX_PATH);

	kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
	int vkCode = kbdStruct.vkCode;

	vInputs.push_back(INPUT());
	int i = vInputs.size() - 1;

	vInputs[i].type = INPUT_KEYBOARD;
	vInputs[i].ki.wVk = vkCode;

	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
		vInputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
	else
		vInputs[i].ki.dwFlags = 0;

	// Logging
	wprintf(L"Keystroke for: %s\nVirtual Keycode: %d\n", lpBaseName, vkCode);

	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		DWORD now = kbdStruct.time;
		INPUT_BELOW_THRESHOLD = calculateTiming(now);
		
		//blanket ignore for first 6 chars
		if (++INPUT_LEN < 6)
			return -1;

		//determine whether to release captured inputs once WINDOW_SIZE is reached
		if (INPUT_LEN == WINDOW_SIZE) {

			ALLOW_INPUT = INPUT_BELOW_THRESHOLD; // set permanent flag to allow keystrokens through
			if (ALLOW_INPUT) {
				releaseHook(); //temporarily unhook in order to properly replay keystrokes
				replayStoredKeystrokes();
				setHook();
			}
			else {
				//continue logging
				return -1;
			}

		}
	}

	return (ALLOW_INPUT ? 0 : -1);
}

void replayStoredKeystrokes() {
	UINT cInputs = vInputs.size();
	PINPUT pInputs = (INPUT*)malloc(sizeof(INPUT) * cInputs);
	for (int i = 0; i < cInputs; ++i) {
		pInputs[i] = vInputs[i];
	}

	int res = SendInput(cInputs, pInputs, sizeof(INPUT));
	vInputs.clear();
}

void keyboardHook() {
	setHook();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
