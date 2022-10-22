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
#include "utils.h"

 // maps keys that are hidden behind the SHIFT layer
std::map<char, char> layeredKeys = {
	{'`', '~'},
	{'1', '!'},
	{'2', '@'},
	{'3', '#'},
	{'4', '$'},
	{'5', '%'},
	{'6', '^'},
	{'7', '&'},
	{'8', '*'},
	{'9', '('},
	{'0', ')'},
	{'[', '{'},
	{']', '}'},
	{'\\', '|'},
	{';', ':'},
	{'\'', '\"'},
	{',', '<'},
	{'.', '>'},
	{'/', '?'},
};

// map to track keystrokes that cannot be mapped with MapVirtualKeyExW
std::map<int, std::string> mapSpecialKeys = {
	{VK_BACK, "BACKSPACE"},
	{VK_RETURN, "\n"},
	{VK_SPACE, " "},
	{VK_TAB, "\t"},
	{VK_LWIN, "WIN"},
	{VK_RWIN, "WIN"},
	{VK_ESCAPE, "ESCAPE"},
	{VK_END, "END"},
	{VK_HOME, "HOME"},
	{VK_LEFT, "LEFT"},
	{VK_RIGHT, "RIGHT"},
	{VK_UP, "UP"},
	{VK_DOWN, "DOWN"},
	{VK_PRIOR, "PG_UP"},
	{VK_NEXT, "PG_DOWN"},
	{VK_OEM_PERIOD, "."},
	{VK_DECIMAL, "."},
	{VK_OEM_PLUS, "+"},
	{VK_OEM_MINUS, "-"},
	{VK_ADD, "+"},
	{VK_SUBTRACT, "-"},
	{VK_INSERT, "INSERT"},
	{VK_DELETE, "DELETE"},
	{VK_PRINT, "PRINT"},
	{VK_SNAPSHOT, "PRINTSCREEN"},
	{VK_SCROLL, "SCROLL"},
	{VK_PAUSE, "PAUSE"},
	{VK_NUMLOCK, "NUMLOCK"},
	{VK_F1, "F1"},
	{VK_F2, "F2"},
	{VK_F3, "F3"},
	{VK_F4, "F4"},
	{VK_F5, "F5"},
	{VK_F6, "F6"},
	{VK_F7, "F7"},
	{VK_F8, "F8"},
	{VK_F9, "F9"},
	{VK_F10, "F10"},
	{VK_F11, "F11"},
	{VK_F12, "F12"},
};

// stores booleans for mod keys
// +-------+------+------+
// | SHIFT | CAPS | CASE |
// +-------+------+------+
// |     0 |    0 |    0 |
// |     0 |    1 |    1 |
// |     1 |    0 |    1 |
// |     1 |    1 |    0 |
// +-------+------+------+
std::map<std::string, bool> modKeyStates = {
	{"SHIFT", 0},
	{"CAPS", 0},
};

std::mutex keyboardHookMutex;
HHOOK ghHook;
KBDLLHOOKSTRUCT kbdStruct;
std::string kbBuffer;
std::vector<INPUT> vInputs;

// flag to toggle whether input is let through
bool ALLOW_INPUT = false;
bool INPUT_BELOW_THRESHOLD = true;
bool NOTIFIED = false;
int INPUT_WINDOW = 0;

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


int setModKeyState(int vkCode, int event){
	std::map<int, std::string> vkCodeToString = {
		{VK_SHIFT, "SHIFT"},
		{VK_LSHIFT, "SHIFT"},
		{VK_RSHIFT, "SHIFT"},
		{VK_CAPITAL, "CAPS"},
		{VK_CONTROL, "CTRL"},
		{VK_LCONTROL, "CTRL"},
		{VK_RCONTROL, "CTRL"},
		{VK_MENU, "ALT"},
		{VK_RMENU, "ALT"},
		{VK_LMENU, "ALT"},
		{VK_LWIN, "WIN"},
		{VK_RWIN, "WIN"},
	};

	if (vkCodeToString.find(vkCode) == vkCodeToString.end())
		// must log, not a mod key
		return 0;

	std::string keyName = vkCodeToString.at(vkCode);

	if (modKeyStates.find(keyName) == modKeyStates.end())
		return 0;

	int isKeyDown = (event == WM_KEYDOWN || event == WM_SYSKEYDOWN);

	if (keyName != "CAPS"){
		modKeyStates.at(keyName) = isKeyDown;
		return 1;
	}

	modKeyStates.at(keyName) = isKeyDown;

	return 1;
}

char formatKey(char key){
    // handles the transforming of keys that are modified using the SHIFT and CapsLock keys

    if (isalpha(key))
        return (modKeyStates.at("SHIFT") ^ modKeyStates.at("CAPS")) ? key : tolower(key);


    if (modKeyStates.at("SHIFT")){
        auto layeredKeyEntry = layeredKeys.find(key);
        return (layeredKeyEntry == layeredKeys.end()) ? key : layeredKeyEntry->second;
    }

    return key;
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

	int isModKey = setModKeyState(vkCode, wParam);

	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {

		if (!isModKey) {
			if (mapSpecialKeys.find(vkCode) != mapSpecialKeys.end()) {
				std::string key = mapSpecialKeys.at(vkCode);

				kbBuffer += key;
			}
			else {
				HKL kbLayout = GetKeyboardLayout(GetCurrentProcessId());

				char key = MapVirtualKeyExA(vkCode, MAPVK_VK_TO_CHAR, kbLayout);
				key = formatKey(key);
				kbBuffer.push_back(key);
			}
		}

		std::cout << kbBuffer << std::endl;

		DWORD now = kbdStruct.time;
		INPUT_BELOW_THRESHOLD = calculateTiming(now);

		//determine whether to release captured inputs once WINDOW_SIZE is reached
		if (++INPUT_WINDOW == WINDOW_SIZE) {
			if (INPUT_BELOW_THRESHOLD && !ALLOW_INPUT) {
				ALLOW_INPUT = true;
				releaseHook(); //temporarily unhook in order to properly replay keystrokes
				replayStoredKeystrokes();
				setHook();
				INPUT_WINDOW = 0;
			}
			else if (INPUT_BELOW_THRESHOLD && ALLOW_INPUT) {
				INPUT_WINDOW = 0;
			}
			//once input is tagged as malicious there is no way to unblock input
			else {
				ALLOW_INPUT = false;
				if (!NOTIFIED) {
					notify(L"A possible HID injection attack has been detected!");
					NOTIFIED = true;
				}
			}
		}
	}

	return (ALLOW_INPUT ? 0 : -1);
}

void replayStoredKeystrokes() {
	UINT cInputs = vInputs.size();
	for (int i = 0; i < cInputs; ++i) {
		INPUT input = vInputs[i];
		INPUT pInputs[1] = { input };

		int res = SendInput(1, pInputs, sizeof(INPUT));

		if ((input.ki.wVk == VK_LWIN || input.ki.wVk == VK_RWIN) && input.ki.dwFlags == KEYEVENTF_KEYUP) {
			Sleep(100);
		}
	}

	vInputs.clear();
}

void keyboardHook() {
	if (keyboardHookMutex.try_lock()) {
		setHook();
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
